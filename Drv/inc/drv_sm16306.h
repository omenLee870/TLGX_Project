#ifndef __DRV_SM16306_H
#define __DRV_SM16306_H

#include "ciu32f003_std.h"
#include <stdbool.h>

/**
 * @brief 向 SM16306 写入 16 位通道数据
 * @param data 位图数据，每一位对应一个通道（bit15对应OUT15，bit0对应OUT0）
 */
void Drv_SM16306_Write(uint16_t data);

/**
 * @brief 控制 SM16306 输出总开关
 * @param enable true: 开启显示(OE拉低), false: 关闭显示(OE拉高)
 */
void Drv_SM16306_DisplayControl(bool enable);

#endif
