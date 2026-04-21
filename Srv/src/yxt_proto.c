#include "yxt_proto.h"
#include "ciu32f003_std_gpio.h"
#include "bsp_gpio.h"

YXT_Data_t g_yxt_data = {0};

static uint16_t s_low_cnt = 0;   // 低电平计数
static uint16_t s_high_cnt = 0;  // 高电平计数
static uint8_t  s_bit_idx = 0;   // 位计数 (0~103)
static uint8_t  s_raw_bytes[13]; // 原始字节数据
static bool     s_frame_ready = false;

// 100us 中断内运行：负责脉冲宽度测量和位识别
void YXT_Sampling_Handler(void) {
    // 使用官方库获取引脚电平
    bool level = std_gpio_get_input_pin(GPIO_YXT_PORT, GPIO_YXT_PIN);

    if (!level) { // 低电平
        if (s_high_cnt > 0) {
            // 高电平结束，识别位。由于 0 是低长高短，1 是低短高长
            if (s_bit_idx < 104) {
                if (s_low_cnt > s_high_cnt) {
                    s_raw_bytes[s_bit_idx / 8] &= ~(1 << (s_bit_idx % 8)); // Bit 0
                } else {
                    s_raw_bytes[s_bit_idx / 8] |= (1 << (s_bit_idx % 8));  // Bit 1
                }
                s_bit_idx++;
            }
            s_high_cnt = 0;
        }
        s_low_cnt++;
    } else { // 高电平
        if (s_low_cnt > 200) { // 同步码判定：长低电平结束
            s_bit_idx = 0;
            s_frame_ready = false;
        }
        s_low_cnt = 0;
        s_high_cnt++;
        
        // 接收满 13 字节且高电平维持，认为一帧结束
        if (s_bit_idx >= 104 && s_high_cnt > 10) {
            s_frame_ready = true;
        }
    }
}

// 调度器任务运行：负责协议解析映射
void YXT_Decode_Task(void) {
    if (!s_frame_ready) return;
    s_frame_ready = false;

    // 1. 校验 Device Code 和校验和
    if (s_raw_bytes[0] != 0x08) return;
    uint8_t xor_sum = 0;
    for (int i = 0; i < 12; i++) xor_sum ^= s_raw_bytes[i];
    if (xor_sum != s_raw_bytes[12]) return;

    // 2. 映射业务数据
    g_yxt_data.speed      = s_raw_bytes[7]; // DATA7 车速
    g_yxt_data.soc        = s_raw_bytes[9] & 0x7F; // DATA9 SOC
    g_yxt_data.is_parking = (s_raw_bytes[2] >> 1) & 0x01; // DATA2_D1 P档
    g_yxt_data.is_brake   = (s_raw_bytes[4] >> 5) & 0x01; // DATA4_D5 刹车 
    
    // 三速解析
    uint8_t gear_bits = s_raw_bytes[4] & 0x03; 
    if (gear_bits == 0x01) g_yxt_data.gear = 1;
    else if (gear_bits == 0x02) g_yxt_data.gear = 2;
    else if (gear_bits == 0x03) g_yxt_data.gear = 3;

    g_yxt_data.is_online = true;
}
