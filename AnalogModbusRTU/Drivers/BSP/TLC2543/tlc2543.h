#ifndef __TLC2543_H__
#define __TLC2543_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* �궨�� */
/******************************************************************************************/
#define ADC_NUM 37            /* ADC SPI1 TLC2543 ͨ���ɼ�����һ����ѯ�����С */
#define ADC_CHANNELS 9        /* ADC ��ȡͨ���� */
#define SLIDING_WINDOW_SIZE 9 /* �����˲����ڴ�С ��Ҫ������ʾ�����˲� */
#define THRES_SHOLD 1         /* �źŴ��� ������С ��Ҫ������ʾ�����˲� */

/* �����¼��� AllWarning_EventHandle */
#define AIN1_OFF_LINE (1 << 0) /* AIN1  -   ���� */
#define AIN2_OFF_LINE (1 << 1) /* AIN2  -   ���� */
#define AIN3_OFF_LINE (1 << 2) /* AIN3  -   ���� */
#define AIN4_OFF_LINE (1 << 3) /* AIN4  -   ���� */
#define AIN5_OFF_LINE (1 << 4) /* AIN5  -   ���� */
#define AIN6_OFF_LINE (1 << 5) /* AIN6  -   ���� */
#define AIN7_OFF_LINE (1 << 6) /* AIN7  -   ���� */
#define AIN8_OFF_LINE (1 << 7) /* AIN8  -   ���� */
#define AIN9_OFF_LINE (1 << 8) /* AIN9  -   ���� */

  /******************************************************************************************/
  extern uint8_t ADC_Channel[ADC_NUM];
  extern uint8_t TxBuffer[2];
  extern uint8_t RxBuffer[2];
  extern volatile uint32_t Atomic_Channel_Marker;

  extern uint16_t ADC_Value[ADC_NUM];

  /******************************************************************************************/
  /* TLC2543 ADC ͨ������ ת��ǰ�Ľ���ṹ�� */
  typedef struct
  {
    uint16_t val[9]; /* ���������� AIN1 AIN2 AIN3 AIN4 AIN5 AIN6 AIN7 AIN8 AIN9 */
  } SensorValue_Struct;

  typedef struct
  {
    uint16_t flag;  /* ���߱�־λ */
    int32_t val[9]; /* ���������� AIN1 AIN2 AIN3 AIN4 AIN5 AIN6 AIN7 AIN8 AIN9 */
  } SensorData_Struct;

  typedef struct
  {
    uint16_t flag;   /* ���߱�־λ */
    int32_t data[9]; /* ���������� AIN1 AIN2 AIN3 AIN4 AIN5 AIN6 AIN7 AIN8 AIN9 */
  } AnalogData_Struct;

  /* �������� */
  /******************************************************************************************/

  void tlc2543_Spi1_Start(void); /* SPI1_DMA ���ݶ�ȡ��ʼ */
  void tlc2543_Spi1_Stop(void);  /* SPI1_DMA ���ݶ�ȡֹͣ */

  void adc_Get_Average(uint16_t *adc_data);    /* ADC 1ms ����4����ѯ��������ƽ��ֵ���� */
  void adc_LowPass_Filter(uint16_t *adc_data); /* ADC һ�׵�ͨ�˲� */
  // void adc_SlidingAvg_Filter(uint16_t *adc_data); /* ����ƽ�� [18~26] һ���˲���ֵ [27~35] ��ʾ���ֽ���ǿ�˲� */
  // void adc_Deadband_Filter(uint16_t *adc_data);   /* [18~26] ��������  [27~35] ��ʾ���ֽ���ǿ�˲� */

  uint16_t adc_ConvertToData(uint16_t *value, int32_t *data); /* ���㴫����ͨ������ֵ */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* Commnuication_Module */
