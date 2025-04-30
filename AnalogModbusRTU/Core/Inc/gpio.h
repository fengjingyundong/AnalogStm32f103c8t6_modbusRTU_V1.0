/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    gpio.h
 * @brief   This file contains all the function prototypes for
 *          the gpio.c file
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
/* LEDï¿½Ë¿Ú¶ï¿½ï¿½ï¿½ */
#define RUN_LED(x)                                                                                                                           \
  do                                                                                                                                         \
  {                                                                                                                                          \
    x ? HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(RUN_LED_GPIO_Port, RUN_LED_Pin, GPIO_PIN_RESET); \
  } while (0) /* LED0 = RED */

/* TLC_CS ¶Ë¿Ú¶¨Òå */
#define TLC_CS(x)                                                                                                                        \
  do                                                                                                                                     \
  {                                                                                                                                      \
    x ? HAL_GPIO_WritePin(TLC_CS_GPIO_Port, TLC_CS_Pin, GPIO_PIN_SET) : HAL_GPIO_WritePin(TLC_CS_GPIO_Port, TLC_CS_Pin, GPIO_PIN_RESET); \
  } while (0)

/* TLC_EOC */
#define TLC_EOC HAL_GPIO_ReadPin(TLC_EOC_GPIO_Port, TLC_EOC_Pin) /* ¶ÁÈ¡KEY_L1Òý½Å */

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

