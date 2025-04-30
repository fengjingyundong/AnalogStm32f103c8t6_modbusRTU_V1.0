#ifndef __CRC_H__
#define __CRC_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

  /* 宏定义 */
  /******************************************************************************************/

  /* 函数声明 */
  /******************************************************************************************/
  int16_t App_Tab_Get_CRC16(unsigned char *puchMsg, unsigned short usDataLen); /* 计算 CRC 冗余效验数据 */


#ifdef __cplusplus
}
#endif

#endif /* __CRC_H__ */
