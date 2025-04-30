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

/* SPI1_TLC2543 ��ȡ����ʹ�ù��������ṩ DMA ����ʹ�� */
uint8_t TxBuffer[2] = {0x00, 0x00};
uint8_t RxBuffer[2] = {0x00, 0x00};
volatile uint32_t Atomic_Channel_Marker = 0;
uint8_t ADC_Channel[ADC_NUM] = {0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
                                0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90};
uint16_t ADC_Value[ADC_NUM] = {0x00}; /* ����ʹ�ñ��� SPI1_TLC2543 ADC 9��ͨ���ɼ���ֵ */

/******************************************************************************************/

/* SPI1_DMA ���ݶ�ȡ��ʼ */
void tlc2543_Spi1_Start(void)
{
  TLC_CS(0);                     // �õ�Ƭѡ�źţ�׼����ʼͨѶ
  HAL_TIM_Base_Start_IT(&htim2); // ������ʱ��
}

/* SPI1_DMA ���ݶ�ȡֹͣ */
void tlc2543_Spi1_Stop(void)
{
  TLC_CS(1);                    // �õ�Ƭѡ�źţ�׼����ʼͨѶ
  HAL_TIM_Base_Stop_IT(&htim2); // ֹͣ��ʱ��
  Atomic_Channel_Marker = 0;    // ����
}

/******************************************************************************************/
/* ADC 1ms ����4����ѯ��������ƽ��ֵ���� */
void adc_Get_Average(uint16_t *adc_data)
{
  uint8_t i, j;
  uint16_t temp;
  uint16_t val_max[9] = {0x00};
  uint16_t val_min[9] = {0x00};
  uint32_t val_temp[9] = {0x00};

  // Initialize min/max with first values
  for (i = 0; i < 9; i++) /* װ�ؼ����ֵ */
  {
    val_max[i] = *(adc_data + i);
    val_min[i] = *(adc_data + i);
  }

  // Collect 4 samples and track min/max
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 9; j++)
    {
      temp = *(adc_data + (i * 9) + j); /* �ݴ����� ������� */
      val_temp[j] += temp;              /* ��� */
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
/* ADC һ�׵�ͨ�˲� [9~17]ԭֵ  [18~26]} ����һ���˲���� */
void adc_LowPass_Filter(uint16_t *adc_data)
{
  uint8_t i;
  float k = 0.6;
  uint32_t temp[9] = {0, 0, 0, 0, 0};
  static uint32_t last_val[9] = {0, 0, 0, 0, 0};

  /* ��ȡ�����ֵ */
  for (i = 0; i < 9; i++)
  {
    temp[i] = *(adc_data + i + 9) * 10;                              /* ��ȡ������Ҫ�������ֵ */
    last_val[i] = (uint32_t)(k * temp[i] + ((1 - k) * last_val[i])); /* �����趨�ı��������� */
    *(adc_data + 18 + i) = (uint16_t)(last_val[i] / 10);             /* ������μ���Ľ�� �´μ���ʹ�� */
  }
}

/* ����ƽ�� [18~26] һ���˲���ֵ [27~35] ��ʾ���ֽ���ǿ�˲� */
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
    buffer[i][index++] = *(adc_data + 18 + i); /* ͨ�� ���ݴ��뻺������ */
    if (index > SLIDING_WINDOW_SIZE)
      index = 0;

    // Calculate average
    sum = 0; /* ����ۼӺ� */
    for (j = 0; j < SLIDING_WINDOW_SIZE; j++)
      sum += buffer[i][j];

    *(adc_data + 27 + i) = (uint16_t)(sum / SLIDING_WINDOW_SIZE);
  }
}

/* [27~35] ��������  [27~35] ��ʾ���ֽ���ǿ�˲� */
void adc_Deadband_Filter(uint16_t *adc_data)
{
  uint8_t i = 0;
  static uint16_t last_val[ADC_CHANNELS] = {0x00};

  for (i = 0; i < 9; i++)
  {
    if (abs(*(adc_data + 18 + i) - last_val[i]) > THRES_SHOLD) /* �����жϴ��� */
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
/* ���㴫����ͨ������ֵ */
uint16_t adc_ConvertToData(uint16_t *value, int32_t *data)
{
  uint8_t i;
  uint16_t flag = 0; /* ���� - ��־λ */

  for (i = 0; i < 9; i++)
  {
    /* AIN1 ���������ݼ���  */
    if (*(value + i) >= 800) /* �жϲ���ֵ�Ƿ�С������λ */
    {
      *(data + i) = (*(value + i) - 800) * (0.01875 * 100); /* ���㴫������ֵ */
    }
    else
      *(data + i) = 0;      /* ������� һ������ */
    if (*(value + i) < 750) /* ���� �ж� */
      flag |= (1 << i);
  }

  return flag; /* ���ض��߱�־λ */
}
