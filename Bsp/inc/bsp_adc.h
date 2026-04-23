#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "ciu32f003_std.h"

/* 定义采样通道 */
#define LIGHT_SENSOR_ADC_CH    ADC_CHANNEL_2  // 对应 PA4

/* 函数声明 */
void BSP_ADC_Init(void);
uint16_t BSP_ADC_ReadValue(void);
void BSP_ADC_Software_Calibrate(void);
void tim3_init(void);
uint32_t bsp_adc_get_adcvalue(void);

#endif
