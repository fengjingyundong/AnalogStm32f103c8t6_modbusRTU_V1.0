#ifndef __TLC2543_H__
#define __TLC2543_H__

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* 宏定义 */
/******************************************************************************************/
#define ADC_NUM 37            /* ADC SPI1 TLC2543 通道采集数据一次轮询数组大小 */
#define ADC_CHANNELS 9        /* ADC 读取通道数 */
#define SLIDING_WINDOW_SIZE 9 /* 滑动滤波窗口大小 主要用于显示部分滤波 */
#define THRES_SHOLD 1         /* 信号处理 死区大小 主要用于显示部分滤波 */

/* 报警事件组 AllWarning_EventHandle */
#define AIN1_OFF_LINE (1 << 0) /* AIN1  -   断线 */
#define AIN2_OFF_LINE (1 << 1) /* AIN2  -   断线 */
#define AIN3_OFF_LINE (1 << 2) /* AIN3  -   断线 */
#define AIN4_OFF_LINE (1 << 3) /* AIN4  -   断线 */
#define AIN5_OFF_LINE (1 << 4) /* AIN5  -   断线 */
#define AIN6_OFF_LINE (1 << 5) /* AIN6  -   断线 */
#define AIN7_OFF_LINE (1 << 6) /* AIN7  -   断线 */
#define AIN8_OFF_LINE (1 << 7) /* AIN8  -   断线 */
#define AIN9_OFF_LINE (1 << 8) /* AIN9  -   断线 */

  /******************************************************************************************/
  extern uint8_t ADC_Channel[ADC_NUM];
  extern uint8_t TxBuffer[2];
  extern uint8_t RxBuffer[2];
  extern volatile uint32_t Atomic_Channel_Marker;

  extern uint16_t ADC_Value[ADC_NUM];

  /******************************************************************************************/
  /* TLC2543 ADC 通道数据 转换前的结果结构体 */
  typedef struct
  {
    uint16_t val[9]; /* 具体数据组 AIN1 AIN2 AIN3 AIN4 AIN5 AIN6 AIN7 AIN8 AIN9 */
  } SensorValue_Struct;

  typedef struct
  {
    uint16_t flag;  /* 断线标志位 */
    int32_t val[9]; /* 具体数据组 AIN1 AIN2 AIN3 AIN4 AIN5 AIN6 AIN7 AIN8 AIN9 */
  } SensorData_Struct;

  typedef struct
  {
    uint16_t flag;   /* 断线标志位 */
    int32_t data[9]; /* 具体数据组 AIN1 AIN2 AIN3 AIN4 AIN5 AIN6 AIN7 AIN8 AIN9 */
  } AnalogData_Struct;

  /* 函数声明 */
  /******************************************************************************************/

  void tlc2543_Spi1_Start(void); /* SPI1_DMA 数据读取开始 */
  void tlc2543_Spi1_Stop(void);  /* SPI1_DMA 数据读取停止 */

  void adc_Get_Average(uint16_t *adc_data);    /* ADC 1ms 毫秒4次轮询采样算数平均值计算 */
  void adc_LowPass_Filter(uint16_t *adc_data); /* ADC 一阶低通滤波 */
  // void adc_SlidingAvg_Filter(uint16_t *adc_data); /* 滑动平均 [18~26] 一节滤波数值 [27~35] 显示部分进行强滤波 */
  // void adc_Deadband_Filter(uint16_t *adc_data);   /* [18~26] 死区过滤  [27~35] 显示部分进行强滤波 */

  uint16_t adc_ConvertToData(uint16_t *value, int32_t *data); /* 计算传感器通道的数值 */
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* Commnuication_Module */
