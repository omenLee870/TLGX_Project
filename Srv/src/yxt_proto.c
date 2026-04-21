#include "yxt_proto.h"
#include "bsp_gpio.h"

static bool s_yxt_is_inited = false;
static uint16_t s_low_cnt = 0;
static uint16_t s_high_cnt = 0;
static uint8_t  s_bit_idx = 0;
static uint8_t  s_raw_bytes[13]; // 协议定义共13字节 
static bool     s_frame_ready = false;

YXT_Data_t g_yxt_data = {0};

/**
 * @brief 协议层初始化
 */
void YXT_Protocol_Init(void) {
    s_low_cnt = 0;
    s_high_cnt = 0;
    s_bit_idx = 0;
    s_frame_ready = false;
    g_yxt_data.is_online = false;
    s_yxt_is_inited = true;
}

/**
 * @brief 100us 硬件采样 (中断运行)
 */
void YXT_Sampling_Handler(void) {
    // 若未初始化，直接跳出，防止对未定义变量操作
    if (!s_yxt_is_inited) return;

    // 采样引脚 PA7 [cite: 4]
    bool level = std_gpio_get_input_pin(GPIO_YXT_PORT, GPIO_YXT_PIN);

    if (!level) { // 低电平阶段
        if (s_high_cnt > 0) {
            if (s_bit_idx < 104) {
                // 依据比例判定逻辑 0 或 1 
                if (s_low_cnt > s_high_cnt) {
                    s_raw_bytes[s_bit_idx / 8] &= ~(1 << (s_bit_idx % 8));
                } else {
                    s_raw_bytes[s_bit_idx / 8] |= (1 << (s_bit_idx % 8));
                }
                s_bit_idx++;
            }
            s_high_cnt = 0;
        }
        s_low_cnt++;
    } else { // 高电平阶段
        if (s_low_cnt > 200) { // 同步码检测 
            s_bit_idx = 0;
            s_frame_ready = false;
        }
        s_low_cnt = 0;
        s_high_cnt++;
        if (s_bit_idx >= 104 && s_high_cnt > 10) s_frame_ready = true;
    }
}

/**
 * @brief 防御性解码任务
 */
void YXT_Decode_Task(void) {
    // 防御性初始化检查
    if (!s_yxt_is_inited) {
        YXT_Protocol_Init();
        return; 
    }

    if (!s_frame_ready) return;
    s_frame_ready = false;

    // 执行校验逻辑 (Device Code: 0x08) 
    if (s_raw_bytes[0] != 0x08) return;

    // 此处根据协议进行 XOR 校验与数据解析 
    // ... (具体解析代码同前)
    g_yxt_data.is_online = true;
}
