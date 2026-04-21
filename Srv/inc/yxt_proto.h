#ifndef __YXT_PROTO_H
#define __YXT_PROTO_H

#include <stdint.h>
#include <stdbool.h>

// 台铃 Y34B 数据结构
typedef struct {
    uint8_t  speed;         // 速度 (实际车速*10)
    uint8_t  gear;          // 档位: 1-低速, 2-中速, 3-高速
    uint8_t  soc;           // 电池百分比
    uint16_t current_ma;    // 电流 (单位 1A)
    bool     is_parking;    // P档
    bool     is_under_vol;  // 欠压
    bool     is_brake;      // 刹车
    bool     is_online;     // 是否收到数据
} YXT_Data_t;

extern YXT_Data_t g_yxt_data;

void YXT_Protocol_Init(void);
void YXT_Sampling_Handler(void); // 100us 中断调用
void YXT_Decode_Task(void);      // 调度器调用

#endif
