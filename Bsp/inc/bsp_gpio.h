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

/* 函数声明 */
void BSP_GPIO_Init(void);

#endif
