#include "modbus.h"
#include "crc.h"
#include "usart.h"
#include "cmsis_os.h"

#include "FreeRTOS.h"
#include "queue.h"

__align(8) uint16_t Modbus_Hold_Reg[MODBUS_REG];

extern osMessageQueueId_t ModbusOneReg_QueueHandle; /* Definitions for ModbusOneReg_Queue */

/* 0x03 ������ �ظ����� */
void Modbus_03Reply(ModbusOneReg_Struct reg)
{
  uint8_t i = 0;
  uint16_t crc16Temp = 0;
  U2Tx_SBuff.buf[0] = SlaveID;        /* 01 �豸��ַ */
  U2Tx_SBuff.buf[1] = reg.command;    /* 03 ������ */
  U2Tx_SBuff.buf[2] = reg.regNum * 2; /* Ӧ�� ��ȡ���ֽ��� */

  for (i = 0; i < reg.regNum; i++)
  {
    U2Tx_SBuff.buf[3 + i * 2] = Modbus_Hold_Reg[reg.startAddr + i] >> 8; /* �߰�λ���� */
    U2Tx_SBuff.buf[4 + i * 2] = Modbus_Hold_Reg[reg.startAddr + i];      /* �Ͱ�λ����  */
  }
  crc16Temp = App_Tab_Get_CRC16(U2Tx_SBuff.buf, reg.regNum * 2 + 3); // ��ȡCRCУ��ֵ
  U2Tx_SBuff.buf[reg.regNum * 2 + 3] = crc16Temp & 0xFF;             // CRC��λ
  U2Tx_SBuff.buf[reg.regNum * 2 + 4] = (crc16Temp >> 8);             // CRC��λ

  HAL_UART_Transmit_DMA(&huart2, U2Tx_SBuff.buf, reg.regNum * 2 + 5); /* U2_DMA ����Ӧ������ */
}

/* 0x06 ������ �ظ����� */
void Modbus_06Reply(ModbusOneReg_Struct reg)
{
  uint16_t crc16Temp = 0;

  Modbus_Hold_Reg[reg.startAddr] = reg.regNum; /* ���ּĴ����� д���Ӧ������ */

  U2Tx_SBuff.buf[0] = SlaveID;            /* 01 �豸��ַ */
  U2Tx_SBuff.buf[1] = reg.command;        /* 03 ������ */
  U2Tx_SBuff.buf[2] = reg.startAddr >> 8; /* д��Ĵ��� ��ַ ��8λ */
  U2Tx_SBuff.buf[3] = reg.startAddr;      /* д��Ĵ��� ��ַ ��8λ */
  U2Tx_SBuff.buf[4] = reg.regNum >> 8;    /* д��Ĵ��� ���� ��8λ */
  U2Tx_SBuff.buf[5] = reg.regNum;         /* д��Ĵ��� ���� ��8λ */

  crc16Temp = App_Tab_Get_CRC16(U2Tx_SBuff.buf, 8 - 2); // ��ȡCRCУ��ֵ
  U2Tx_SBuff.buf[6] = crc16Temp & 0xFF;                 // CRC��λ
  U2Tx_SBuff.buf[7] = (crc16Temp >> 8);                 // CRC��λ

  HAL_UART_Transmit_DMA(&huart2, U2Tx_SBuff.buf, 8); /* U2_DMA ����Ӧ������ */
}

/* Modbus-RTU ���ݽ��� */
void Modbus_Analysis(uint8_t *rx_data, uint8_t len)
{
  static uint8_t tx_data[5];
  uint8_t err = 0;
  uint16_t CRC16 = 0, CRC16Temp = 0;
  uint16_t startRegAddr, regNum; /* MODBUS-RTU  ������ʼ��ַ */
  static ModbusOneReg_Struct modbusSOneReg;
  BaseType_t queueErr;
  if (rx_data[0] == SlaveID) // ��ַ���ڱ�����ַ ��ַ��Χ:1 - 32
  {
    CRC16 = App_Tab_Get_CRC16(rx_data, len - 2); // CRCУ�� ���ֽ���ǰ ���ֽ��ں� ���ֽ�Ϊ�������һ���ֽ�
    CRC16Temp = ((uint16_t)(rx_data[len - 1] << 8) | rx_data[len - 2]);
    // printf("C_CRC:%04X, Rx_CRC:%04X\r\n", CRC16, CRC16Temp);
    if (CRC16 != CRC16Temp)
    {
      err = 4; // CRCУ�����
    }
    startRegAddr = (uint16_t)(rx_data[2] << 8) | rx_data[3];
    if (startRegAddr > MODBUS_REG - 1) /* �ж���ʼ��ַ��Χ�Ƿ񳬷�Χ */
    {
      err = 2; // ��ʼ��ַ���ڹ涨��Χ�� 00 - 03    1 - 4��ͨ��
    }
    if (err == 0)
    {
      switch (rx_data[1]) // ������
      {
      case 3: // ������Ĵ���
      {
        regNum = (uint16_t)(rx_data[4] << 8) | rx_data[5]; /* ��ȡ�Ĵ������� */
        if ((startRegAddr + regNum) <= MODBUS_REG)         /* �Ĵ�����ַ+�Ĵ������� �ڹ涨��Χ�� */
        {
          modbusSOneReg.command = rx_data[1];                                  /* ������ 0x03 */
          modbusSOneReg.startAddr = startRegAddr;                              /* ��ʼ��ַ */
          modbusSOneReg.regNum = regNum;                                       /* �Ĵ������� */
          queueErr = xQueueSend(ModbusOneReg_QueueHandle, &modbusSOneReg, 10); /* ����0x03 ���������� */
          if (queueErr != pdTRUE)
            printf("Err: ModbusOneReg_QueueHandle Send 0x03 Faile !\r\n");
        }
        else
        {
          err = 3; /* �Ĵ����������ڹ涨��Χ�� */
        }

        break;
      }
      case 6: // д�����Ĵ���
      {
        regNum = (uint16_t)(rx_data[4] << 8) | rx_data[5];                   /* ��ȡ�Ĵ������� */
        modbusSOneReg.command = rx_data[1];                                  /* ������ 0x03 */
        modbusSOneReg.startAddr = startRegAddr;                              /* Addr��ַ */
        modbusSOneReg.regNum = regNum;                                       /* �Ĵ������� */
        queueErr = xQueueSend(ModbusOneReg_QueueHandle, &modbusSOneReg, 10); /* ����0x03 ���������� */
        if (queueErr != pdTRUE)
          printf("Err: ModbusOneReg_QueueHandle Send 0x06 Faile !\r\n");
        break;
      }
      case 16: // д����Ĵ���
      {
        // Modbus_16_Slave();
        err = 1; // ��֧�ָù�����
        break;
      }
      default:
      {
        err = 1; // ��֧�ָù�����
        break;
      }
      }
    }
    if (err > 0)
    {
      tx_data[0] = rx_data[0];
      tx_data[1] = rx_data[1] | 0x80;
      tx_data[2] = err;                          // ���ʹ������
      CRC16Temp = App_Tab_Get_CRC16(tx_data, 3); // ����CRCУ��ֵ
      tx_data[3] = CRC16Temp & 0xFF;             // CRC��λ
      tx_data[4] = (CRC16Temp >> 8);             // CRC��λ
      HAL_UART_Transmit_DMA(&huart2, tx_data, 5);
      err = 0; // ���������ݺ���������־
    }
  }
}
