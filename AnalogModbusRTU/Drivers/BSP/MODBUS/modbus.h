#ifndef __MODBUS_H__
#define __MODBUS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* �궨�� */
  /******************************************************************************************/

#define SlaveID 1    /* Modbus_RTU  ��ַ 01 */
#define MODBUS_REG 9 /* Modbus-RTU �Ĵ�����Χ */

  extern uint16_t Modbus_Hold_Reg[MODBUS_REG];

  typedef struct
  {
    uint8_t command;    /* ������ 0x03 0x06  */
    uint16_t startAddr; /* �Ĵ��� ��ʼ��ַ */
    uint16_t regNum;    /* ��Ӧ0x03 ������� �Ĵ����������  0x06 ������ �Ĵ������� */
  } ModbusOneReg_Struct;

  /* �������� */
  /******************************************************************************************/

  void Modbus_03Reply(ModbusOneReg_Struct reg);        /* 0x03 ������ �ظ����� */
  void Modbus_06Reply(ModbusOneReg_Struct reg);        /* 0x06 ������ �ظ����� */
  void Modbus_Analysis(uint8_t *rx_data, uint8_t len); /* Modbus-RTU ���ݽ��� */

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_H__ */
