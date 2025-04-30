#include "modbus.h"
#include "crc.h"
#include "usart.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "queue.h"

__align(8) uint16_t Modbus_Hold_Reg[MODBUS_REG];

extern osMessageQueueId_t ModbusOneReg_QueueHandle; /* Definitions for ModbusOneReg_Queue */

/* 0x03 功能码 回复函数 */
void Modbus_03Reply(ModbusOneReg_Struct reg)
{
  uint8_t i = 0;
  uint16_t crc16Temp = 0;
  U2Tx_SBuff.buf[0] = SlaveID;        /* 01 设备地址 */
  U2Tx_SBuff.buf[1] = reg.command;    /* 03 功能码 */
  U2Tx_SBuff.buf[2] = reg.regNum * 2; /* 应答 读取的字节数 */

  for (i = 0; i < reg.regNum; i++)
  {
    U2Tx_SBuff.buf[3 + i * 2] = Modbus_Hold_Reg[reg.startAddr + i] >> 8; /* 高八位数据 */
    U2Tx_SBuff.buf[4 + i * 2] = Modbus_Hold_Reg[reg.startAddr + i];      /* 低八位数据  */
  }
  crc16Temp = App_Tab_Get_CRC16(U2Tx_SBuff.buf, reg.regNum * 2 + 3); // 获取CRC校验值
  U2Tx_SBuff.buf[reg.regNum * 2 + 3] = crc16Temp & 0xFF;             // CRC低位
  U2Tx_SBuff.buf[reg.regNum * 2 + 4] = (crc16Temp >> 8);             // CRC高位

  HAL_UART_Transmit_DMA(&huart2, U2Tx_SBuff.buf, reg.regNum * 2 + 5); /* U2_DMA 发送应答数据 */
}

/* 0x06 功能码 回复函数 */
void Modbus_06Reply(ModbusOneReg_Struct reg)
{
  uint16_t crc16Temp = 0;

  Modbus_Hold_Reg[reg.startAddr] = reg.regNum; /* 保持寄存器组 写入对应的数据 */

  U2Tx_SBuff.buf[0] = SlaveID;            /* 01 设备地址 */
  U2Tx_SBuff.buf[1] = reg.command;        /* 03 功能码 */
  U2Tx_SBuff.buf[2] = reg.startAddr >> 8; /* 写入寄存器 地址 高8位 */
  U2Tx_SBuff.buf[3] = reg.startAddr;      /* 写入寄存器 地址 低8位 */
  U2Tx_SBuff.buf[4] = reg.regNum >> 8;    /* 写入寄存器 数据 高8位 */
  U2Tx_SBuff.buf[5] = reg.regNum;         /* 写入寄存器 数据 低8位 */

  crc16Temp = App_Tab_Get_CRC16(U2Tx_SBuff.buf, 8 - 2); // 获取CRC校验值
  U2Tx_SBuff.buf[6] = crc16Temp & 0xFF;                 // CRC低位
  U2Tx_SBuff.buf[7] = (crc16Temp >> 8);                 // CRC高位

  HAL_UART_Transmit_DMA(&huart2, U2Tx_SBuff.buf, 8); /* U2_DMA 发送应答数据 */
}

/* Modbus-RTU 数据解析 */
void Modbus_Analysis(uint8_t *rx_data, uint8_t len)
{
  static uint8_t tx_data[5];
  uint8_t err = 0;
  uint16_t CRC16 = 0, CRC16Temp = 0;
  uint16_t startRegAddr, regNum; /* MODBUS-RTU  数据起始地址 */
  static ModbusOneReg_Struct modbusSOneReg;
  BaseType_t queueErr;
  if (rx_data[0] == SlaveID) // 地址等于本机地址 地址范围:1 - 32
  {
    CRC16 = App_Tab_Get_CRC16(rx_data, len - 2); // CRC校验 低字节在前 高字节在后 高字节为报文最后一个字节
    CRC16Temp = ((uint16_t)(rx_data[len - 1] << 8) | rx_data[len - 2]);
    // printf("C_CRC:%04X, Rx_CRC:%04X\r\n", CRC16, CRC16Temp);
    if (CRC16 != CRC16Temp)
    {
      err = 4; // CRC校验错误
    }
    startRegAddr = (uint16_t)(rx_data[2] << 8) | rx_data[3];
    if (startRegAddr > MODBUS_REG - 1) /* 判断起始地址范围是否超范围 */
    {
      err = 2; // 起始地址不在规定范围内 00 - 03    1 - 4号通道
    }
    if (err == 0)
    {
      switch (rx_data[1]) // 功能码
      {
      case 3: // 读多个寄存器
      {
        regNum = (uint16_t)(rx_data[4] << 8) | rx_data[5]; /* 获取寄存器数量 */
        if ((startRegAddr + regNum) <= MODBUS_REG)         /* 寄存器地址+寄存器数量 在规定范围内 */
        {
          modbusSOneReg.command = rx_data[1];                                  /* 功能码 0x03 */
          modbusSOneReg.startAddr = startRegAddr;                              /* 起始地址 */
          modbusSOneReg.regNum = regNum;                                       /* 寄存器个数 */
          queueErr = xQueueSend(ModbusOneReg_QueueHandle, &modbusSOneReg, 10); /* 发送0x03 单命令数据 */
          if (queueErr != pdTRUE)
            printf("Err: ModbusOneReg_QueueHandle Send 0x03 Faile !\r\n");
        }
        else
        {
          err = 3; /* 寄存器数量不在规定范围内 */
        }

        break;
      }
      case 6: // 写单个寄存器
      {
        regNum = (uint16_t)(rx_data[4] << 8) | rx_data[5];                   /* 获取寄存器数据 */
        modbusSOneReg.command = rx_data[1];                                  /* 功能码 0x03 */
        modbusSOneReg.startAddr = startRegAddr;                              /* Addr地址 */
        modbusSOneReg.regNum = regNum;                                       /* 寄存器数据 */
        queueErr = xQueueSend(ModbusOneReg_QueueHandle, &modbusSOneReg, 10); /* 发送0x03 单命令数据 */
        if (queueErr != pdTRUE)
          printf("Err: ModbusOneReg_QueueHandle Send 0x06 Faile !\r\n");
        break;
      }
      case 16: // 写多个寄存器
      {
        // Modbus_16_Slave();
        err = 1; // 不支持该功能码
        break;
      }
      default:
      {
        err = 1; // 不支持该功能码
        break;
      }
      }
    }
    if (err > 0)
    {
      tx_data[0] = rx_data[0];
      tx_data[1] = rx_data[1] | 0x80;
      tx_data[2] = err;                          // 发送错误代码
      CRC16Temp = App_Tab_Get_CRC16(tx_data, 3); // 计算CRC校验值
      tx_data[3] = CRC16Temp & 0xFF;             // CRC低位
      tx_data[4] = (CRC16Temp >> 8);             // CRC高位
      HAL_UART_Transmit_DMA(&huart2, tx_data, 5);
      err = 0; // 发送完数据后清除错误标志
    }
  }
}
