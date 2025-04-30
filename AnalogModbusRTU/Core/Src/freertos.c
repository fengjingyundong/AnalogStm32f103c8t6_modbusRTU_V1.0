/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
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

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio.h"
#include "usart.h"
#include "tim.h"
#include "spi.h"
#include "iwdg.h"
#include "tlc2543.h"
#include "modbus.h"

#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"

#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define MODBUS_ONEREG_QUEUE_LENGHT 10   /* MODBUS OneReg 命令传输 队列深度 */
#define ANALOG_INPUTDATA_QUEUE_LENGHT 1 /* 模拟量输入压力 数据传输 队列深度 */
#define QUEUESET_LENGHT MODBUS_ONEREG_QUEUE_LENGHT + ANALOG_INPUTDATA_QUEUE_LENGHT
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

static QueueSetHandle_t QueueSet_Handle; /* 声明队列集句柄 */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name = "defaultTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for RUNTask */
osThreadId_t RUNTaskHandle;
const osThreadAttr_t RUNTask_attributes = {
    .name = "RUNTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for ADCReadTask */
osThreadId_t ADCReadTaskHandle;
const osThreadAttr_t ADCReadTask_attributes = {
    .name = "ADCReadTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for IWDGRefreshTask */
osThreadId_t IWDGRefreshTaskHandle;
const osThreadAttr_t IWDGRefreshTask_attributes = {
    .name = "IWDGRefreshTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for Usart2TxRxTask */
osThreadId_t Usart2TxRxTaskHandle;
const osThreadAttr_t Usart2TxRxTask_attributes = {
    .name = "Usart2TxRxTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
/* Definitions for HoldRegTask */
osThreadId_t HoldRegTaskHandle;
const osThreadAttr_t HoldRegTask_attributes = {
    .name = "HoldRegTask",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* Definitions for ADC_Queue */
osMessageQueueId_t ADC_QueueHandle;
const osMessageQueueAttr_t ADC_Queue_attributes = {
    .name = "ADC_Queue"};
/* Definitions for U2Rx_Queue */
osMessageQueueId_t U2Rx_QueueHandle;
const osMessageQueueAttr_t U2Rx_Queue_attributes = {
    .name = "U2Rx_Queue"};
/* Definitions for ModbusOneReg_Queue */
osMessageQueueId_t ModbusOneReg_QueueHandle;
const osMessageQueueAttr_t ModbusOneReg_Queue_attributes = {
    .name = "ModbusOneReg_Queue"};
/* Definitions for AnalogInputDataQueue */
osMessageQueueId_t AnalogInputDataQueueHandle;
const osMessageQueueAttr_t AnalogInputDataQueue_attributes = {
    .name = "AnalogInputDataQueue"};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void Run_LedTask(void *argument);
void ADC_ReadTask(void *argument);
void IWDG_RefreshTask(void *argument);
void Usart2TxRx_Task(void *argument);
void HoldReg_Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
  /* USER CODE BEGIN Init */
  BaseType_t err = pdFAIL;
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */

  QueueSet_Handle = xQueueCreateSet(QUEUESET_LENGHT); /* 创建 任务集 */
  if (QueueSet_Handle == NULL)
    printf("QueueSet_Handle Create False !\r\n");

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of ADC_Queue */
  ADC_QueueHandle = osMessageQueueNew(1, sizeof(ADC_Value), &ADC_Queue_attributes);

  /* creation of U2Rx_Queue */
  U2Rx_QueueHandle = osMessageQueueNew(16, sizeof(U2Rx_SBuff), &U2Rx_Queue_attributes);

  /* creation of ModbusOneReg_Queue */
  ModbusOneReg_QueueHandle = osMessageQueueNew(10, sizeof(ModbusOneReg_Struct), &ModbusOneReg_Queue_attributes);

  /* creation of AnalogInputDataQueue */
  AnalogInputDataQueueHandle = osMessageQueueNew(1, sizeof(AnalogData_Struct), &AnalogInputDataQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  err = xQueueAddToSet(ModbusOneReg_QueueHandle, QueueSet_Handle);
  if (err != pdTRUE)
    printf("ModbusOneReg_QueueHandle AddToSet False!\r\n");
  err = xQueueAddToSet(AnalogInputDataQueueHandle, QueueSet_Handle);
  if (err != pdTRUE)
    printf("AnalogInputDataQueueHandle AddToSet False!\r\n");

  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of RUNTask */
  RUNTaskHandle = osThreadNew(Run_LedTask, NULL, &RUNTask_attributes);

  /* creation of ADCReadTask */
  ADCReadTaskHandle = osThreadNew(ADC_ReadTask, NULL, &ADCReadTask_attributes);

  /* creation of IWDGRefreshTask */
  IWDGRefreshTaskHandle = osThreadNew(IWDG_RefreshTask, NULL, &IWDGRefreshTask_attributes);

  /* creation of Usart2TxRxTask */
  Usart2TxRxTaskHandle = osThreadNew(Usart2TxRx_Task, NULL, &Usart2TxRxTask_attributes);

  /* creation of HoldRegTask */
  HoldRegTaskHandle = osThreadNew(HoldReg_Task, NULL, &HoldRegTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for (;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_Run_LedTask */
/**
 * @brief Function implementing the RUNTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Run_LedTask */
void Run_LedTask(void *argument)
{
  /* USER CODE BEGIN Run_LedTask */
  /* Infinite loop */
  for (;;)
  {
    RUN_LED(0);
    osDelay(50);
    RUN_LED(1);
    osDelay(50);
    RUN_LED(0);
    osDelay(50);
    RUN_LED(1);
    osDelay(800);
  }
  /* USER CODE END Run_LedTask */
}

/* USER CODE BEGIN Header_ADC_ReadTask */
/**
 * @brief Function implementing the ADCReadTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_ADC_ReadTask */
void ADC_ReadTask(void *argument)
{
  /* USER CODE BEGIN ADC_ReadTask */
  /* Infinite loop */
  BaseType_t err;
  static uint16_t adc_data[ADC_NUM];
  static AnalogData_Struct analog_data;

  osDelay(1000);
  tlc2543_Spi1_Start(); /* SPI1_DMA 数据读取开始 */
  for (;;)
  {
    err = xQueueReceive(ADC_QueueHandle, adc_data, 1000);
    if (err != pdTRUE)
    {
      printf("ADC_QueueHandle Receive false !\r\n");
      tlc2543_Spi1_Stop();  /* SPI1_DMA 数据读取停止 */
      tlc2543_Spi1_Start(); /* SPI1_DMA 数据读取开始 */
    }

    adc_Get_Average(adc_data);    /* ADC 1ms 毫秒4次轮询采样算数平均值计算 */
    adc_LowPass_Filter(adc_data); /* ADC 一阶低通滤波 [9~17]原值  [18~26]} 数字一阶滤波结果 */
    // static uint16_t data[10] = {0x00};
    // data[0] = 0x0201;
    // data[1] = adc_data[0 + 18];
    // data[2] = adc_data[1 + 18];
    // data[3] = adc_data[2 + 18];
    // data[4] = adc_data[3 + 18];
    // data[5] = adc_data[4 + 18];
    // data[6] = adc_data[5 + 18];
    // data[7] = adc_data[6 + 18];
    // data[8] = adc_data[7 + 18];
    // data[9] = adc_data[8 + 18];
    // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)data, sizeof(data));

    adc_ConvertToData(adc_data + 18, analog_data.data);
    // static uint16_t data[10] = {0x00};
    // data[0] = 0x0201;
    // data[1] = analog_data.data[0];
    // data[2] = analog_data.data[1];
    // data[3] = analog_data.data[2];
    // data[4] = analog_data.data[3];
    // data[5] = analog_data.data[4];
    // data[6] = analog_data.data[5];
    // data[7] = analog_data.data[6];
    // data[8] = analog_data.data[7];
    // data[9] = analog_data.data[8];
    // HAL_UART_Transmit_DMA(&huart1, (uint8_t *)analog_data.data, sizeof(analog_data.data));
    err = xQueueOverwrite(AnalogInputDataQueueHandle, &analog_data);
    if (err != pdTRUE)
      printf("AnalogInputDataQueueHandle OverWrite false!\r\n");
    osDelay(10);
  }
  /* USER CODE END ADC_ReadTask */
}

/* USER CODE BEGIN Header_IWDG_RefreshTask */
/**
 * @brief Function implementing the IWDGRefreshTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_IWDG_RefreshTask */
void IWDG_RefreshTask(void *argument)
{
  /* USER CODE BEGIN IWDG_RefreshTask */
  /* Infinite loop */
  for (;;)
  {
    HAL_IWDG_Refresh(&hiwdg);
    osDelay(200);
  }
  /* USER CODE END IWDG_RefreshTask */
}

/* USER CODE BEGIN Header_Usart2TxRx_Task */
/**
 * @brief Function implementing the Usart2TxRxTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_Usart2TxRx_Task */
void Usart2TxRx_Task(void *argument)
{
  /* USER CODE BEGIN Usart2TxRx_Task */
  /* Infinite loop */
  // uint8_t i;
  static U2Rx_Struct rx_sbuff;                                       /* 串口接受局部变量结构体 */
  HAL_UART_Receive_DMA(&huart2, U2Rx_SBuff.buf, USART2_RXBUFF_SIZE); /* 启动DMA 串口数据接收 8 字节 */
  xQueueReset(U2Rx_QueueHandle);
  for (;;)
  {
    xQueueReceive(U2Rx_QueueHandle, &rx_sbuff, portMAX_DELAY); /* 接收 串口空闲中断传输的参数 */
    // printf("%d\t", rx_sbuff.len);
    // for (i = 0; i < rx_sbuff.len - 1; i++)
    //   printf("%02X ", rx_sbuff.buf[i]);
    // printf("%02X\r\n", rx_sbuff.buf[i]);

    Modbus_Analysis((uint8_t *)rx_sbuff.buf, (uint8_t)rx_sbuff.len); /* Modbus-RTU 数据解析 */

    osDelay(10);
  }
  /* USER CODE END Usart2TxRx_Task */
}

/* USER CODE BEGIN Header_HoldReg_Task */
/**
 * @brief Function implementing the HoldRegTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_HoldReg_Task */
void HoldReg_Task(void *argument)
{
  /* USER CODE BEGIN HoldReg_Task */
  /* Infinite loop */
  static ModbusOneReg_Struct modSReg;
  static AnalogData_Struct analogData;
  QueueSetMemberHandle_t activate_member;

  for (;;)
  {
    activate_member = xQueueSelectFromSet(QueueSet_Handle, portMAX_DELAY); /* 阻塞等待 队列集 */
    if (activate_member == ModbusOneReg_QueueHandle)
    {
      xQueueReceive(ModbusOneReg_QueueHandle, &modSReg, portMAX_DELAY); /* 队列数据长度 ModbusOneReg_Struct结构体 */
      // taskENTER_CRITICAL();
      // printf("%d %d %d\r\n", modSReg.command, modSReg.startAddr, modSReg.regNum);
      // taskEXIT_CRITICAL();
      if (modSReg.command == 0x03) /* 0x03 读取寄存器功能码 */
      {
        Modbus_03Reply(modSReg); /* 0x03 功能码 回复函数 */
      }
      else if (modSReg.command == 0x06) /* 0x06 单个写入寄存器功能码 */
      {
        Modbus_06Reply(modSReg); /* 0x06 功能码 回复函数 */
      }
    }
    else if (activate_member == AnalogInputDataQueueHandle)
    {
      xQueueReceive(AnalogInputDataQueueHandle, &analogData, portMAX_DELAY); /* 队列数据长度 AnalogData_Struct 结构体 */

      // printf("%d\r\n", analogData.data[0]);
      for(uint8_t i =0;i<MODBUS_REG;i++)
      {
        Modbus_Hold_Reg[i] = (uint16_t)analogData.data[i];
      }
      
    }

    osDelay(1);
  }
  /* USER CODE END HoldReg_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
