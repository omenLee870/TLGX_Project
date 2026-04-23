#ifndef __BSP_GPIO_H
#define __BSP_GPIO_H

#include "ciu32f003_std.h"
#include "sys_config.h"

/* --- 引脚配置 --- */
/* --- SM16306 引脚定义 --- */
#define GPIO_SM_PORT_A      GPIOA
#define GPIO_SM_PORT_B      GPIOB

#define PIN_SM_CLK          GPIO_PIN_0  // PA0
#define PIN_SM_LE           GPIO_PIN_1  // PA1
#define PIN_SM_SDI          GPIO_PIN_0  // PB0
#define PIN_SM_OE           GPIO_PIN_6  // PA6

// --- 一线通(YXT)采样引脚定义 ---
#define GPIO_YXT_PORT       GPIOA
#define GPIO_YXT_PIN        GPIO_PIN_7

// --- 扫描组供电控制 (COM端 - PMOS控制) ---
#define GPIO_SCAN_PORT      GPIOB
#define GPIO_SCAN_A_PIN     GPIO_PIN_1
#define GPIO_SCAN_B_PIN     GPIO_PIN_2
#define GPIO_SCAN_C_PIN     GPIO_PIN_3

// --- 大灯控制引脚 ---
#define GPIO_HEADLIGHT_PORT GPIOB
#define GPIO_HEADLIGHT_PIN  GPIO_PIN_4  // DD-MCU

// --- adc检测引脚 ---
#define GPIO_ADC_PORT        GPIOA
#define GPIO_ADC_PIN         GPIO_PIN_4   // 光敏电阻采样
#define ADC_LIGHT_CH         ADC_CHANNEL_2

/* 函数声明 */
void BSP_GPIO_Init(void);

#endif
