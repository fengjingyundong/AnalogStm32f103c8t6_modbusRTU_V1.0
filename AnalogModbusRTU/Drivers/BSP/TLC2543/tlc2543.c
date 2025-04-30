#include "tlc2543.h"
#include "stdbool.h"
#include "main.h"
#include "usart.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "cmsis_os.h"
#include "stdlib.h"

/* SPI1_TLC2543 读取数据使用公共变量提供 DMA 传输使用 */
uint8_t TxBuffer[2] = {0x00, 0x00};
uint8_t RxBuffer[2] = {0x00, 0x00};
volatile uint32_t Atomic_Channel_Marker = 0;
uint8_t ADC_Channel[ADC_NUM] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90};
uint16_t ADC_Value[ADC_NUM] = {0x00}; /* 队列使用变量 SPI1_TLC2543 ADC 9个通道采集数值 */

/******************************************************************************************/

/* SPI1_DMA 数据读取开始 */
void tlc2543_Spi1_Start(void)
{
  TLC_CS(0);                     // 置低片选信号，准备开始通讯
  HAL_TIM_Base_Start_IT(&htim2); // 启动定时器
}

/* SPI1_DMA 数据读取停止 */
void tlc2543_Spi1_Stop(void)
{
  TLC_CS(1);                    // 置低片选信号，准备开始通讯
  HAL_TIM_Base_Stop_IT(&htim2); // 停止定时器
  Atomic_Channel_Marker = 0;    // 清零
}

/******************************************************************************************/
/* ADC 1ms 毫秒4次轮询采样算数平均值计算 */
void adc_Get_Average(uint16_t *adc_data)
{
  uint8_t i, j;
  uint16_t temp;
  uint16_t val_max[9] = {0x00};
  uint16_t val_min[9] = {0x00};
  uint32_t val_temp[9] = {0x00};

  // Initialize min/max with first values
  for (i = 0; i < 9; i++) /* 装载计算初值 */
  {
    val_max[i] = *(adc_data + i);
    val_min[i] = *(adc_data + i);
  }

  // Collect 4 samples and track min/max
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 9; j++)
    {
      temp = *(adc_data + (i * 9) + j); /* 暂存数据 方便计算 */
      val_temp[j] += temp;              /* 求和 */
      if (temp < val_min[j])
        val_min[j] = temp;
      if (temp > val_max[j])
        val_max[j] = temp;
    }
  }

  // Calculate average of middle values
  for (i = 0; i < 9; i++)
  {
    val_temp[i] = val_temp[i] - val_max[i] - val_min[i];
    *(adc_data + i + 9) = (uint16_t)(val_temp[i] / 2);
  }
}

/******************************************************************************************/
/* ADC 一阶低通滤波 [9~17]原值  [18~26]} 数字一阶滤波结果 */
void adc_LowPass_Filter(uint16_t *adc_data)
{
  uint8_t i;
  float k = 0.6;
  uint32_t temp[9] = {0, 0, 0, 0, 0};
  static uint32_t last_val[9] = {0, 0, 0, 0, 0};

  /* 提取计算初值 */
  for (i = 0; i < 9; i++)
  {
    temp[i] = *(adc_data + i + 9) * 10;                              /* 提取当次需要计算的数值 */
    last_val[i] = (uint32_t)(k * temp[i] + ((1 - k) * last_val[i])); /* 按照设定的比例计算结果 */
    *(adc_data + 18 + i) = (uint16_t)(last_val[i] / 10);             /* 保存这次计算的结果 下次计算使用 */
  }
}

/* 滑动平均 [18~26] 一节滤波数值 [27~35] 显示部分进行强滤波 */
void adc_SlidingAvg_Filter(uint16_t *adc_data)
{
  uint8_t i, j;
  uint32_t sum = 0;
  static uint8_t index = 0;
  static uint16_t buffer[9][SLIDING_WINDOW_SIZE] = {0x00};
  static bool is_initialized = false;

  // Input validation
  if (!adc_data)
    return;

  // Initialize buffer on first call
  if (!is_initialized)
  {
    for (i = 0; i < ADC_CHANNELS; i++)
    {
      for (j = 0; j < SLIDING_WINDOW_SIZE; j++)
      {
        buffer[i][j] = *(adc_data + 18 + i);
      }
    }
    is_initialized = true;
  }

  // Process each channel
  for (i = 0; i < ADC_CHANNELS; i++)
  {
    buffer[i][index++] = *(adc_data + 18 + i); /* 通道 数据存入缓冲数组 */
    if (index > SLIDING_WINDOW_SIZE)
      index = 0;

    // Calculate average
    sum = 0; /* 清除累加和 */
    for (j = 0; j < SLIDING_WINDOW_SIZE; j++)
      sum += buffer[i][j];

    *(adc_data + 27 + i) = (uint16_t)(sum / SLIDING_WINDOW_SIZE);
  }
}

/* [27~35] 死区过滤  [27~35] 显示部分进行强滤波 */
void adc_Deadband_Filter(uint16_t *adc_data)
{
  uint8_t i = 0;
  static uint16_t last_val[ADC_CHANNELS] = {0x00};

  for (i = 0; i < 9; i++)
  {
    if (abs(*(adc_data + 18 + i) - last_val[i]) > THRES_SHOLD) /* 死区判断处理 */
    {
      last_val[i] = *(adc_data + 18 + i);
      *(adc_data + 27 + i) = last_val[i];
    }
    else
    {
      *(adc_data + 27 + i) = last_val[i];
    }
  }
}

/******************************************************************************************/
/* 计算传感器通道的数值 */
uint16_t adc_ConvertToData(uint16_t *value, int32_t *data)
{
  uint8_t i;
  uint16_t flag = 0; /* 断线 - 标志位 */

  for (i = 0; i < 9; i++)
  {
    /* AIN1 传感器数据计算  */
    if (*(value + i) >= 800) /* 判断测量值是否小于零点电位 */
    {
      *(data + i) = (*(value + i) - 800) * (0.01875 * 100); /* 计算传感器数值 */
    }
    else
      *(data + i) = 0;      /* 低于零点 一律置零 */
    if (*(value + i) < 750) /* 断线 判断 */
      flag |= (1 << i);
  }

  return flag; /* 返回断线标志位 */
}
