#ifndef __MODBUS_H__
#define __MODBUS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* 宏定义 */
  /******************************************************************************************/

#define SlaveID 1    /* Modbus_RTU  地址 01 */
#define MODBUS_REG 9 /* Modbus-RTU 寄存器范围 */

  extern uint16_t Modbus_Hold_Reg[MODBUS_REG];

  typedef struct
  {
    uint8_t command;    /* 命令码 0x03 0x06  */
    uint16_t startAddr; /* 寄存器 起始地址 */
    uint16_t regNum;    /* 对应0x03 功能码的 寄存器请求个数  0x06 功能码 寄存器数据 */
  } ModbusOneReg_Struct;

  /* 函数声明 */
  /******************************************************************************************/

  void Modbus_03Reply(ModbusOneReg_Struct reg);        /* 0x03 功能码 回复函数 */
  void Modbus_06Reply(ModbusOneReg_Struct reg);        /* 0x06 功能码 回复函数 */
  void Modbus_Analysis(uint8_t *rx_data, uint8_t len); /* Modbus-RTU 数据解析 */

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H__ */
