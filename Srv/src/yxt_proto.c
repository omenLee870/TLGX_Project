#include "yxt_proto.h"
#include "bsp_gpio.h"

// 1. 速度换算系数 (对应你代码中的 C_YXT_SPEED，用于霍尔脉冲模式)
#ifndef C_YXT_SPEED
#define C_YXT_SPEED        15  // 基础比例系数，调大则显示速度变慢，调小则变快
#endif

#ifndef R_YXT_SPEED_SET
#define R_YXT_SPEED_SET
#endif

// 2. SOC 电量格划分区间 (0-100%)
#define SOC_LEVEL_1_LIMIT  20  // 第一格临界点
#define SOC_LEVEL_2_LIMIT  40  // 第二格临界点
#define SOC_LEVEL_3_LIMIT  60  // 第三格临界点
#define SOC_LEVEL_4_LIMIT  80  // 第四格临界点
#define SOC_LEVEL_5_LIMIT  100 // 第五格临界点

static struct {
    uint8_t  step;
    uint16_t low_cnt;
    uint16_t high_cnt;
    uint16_t timeout_cnt;
    uint8_t  buf[13];       // 严格存储 13 字节 (DATA0 - DATA12)
    bool     frame_ready;
    bool     old_level;
} s_yxt_core = {0};

YXT_Data_t g_yxt_data = {0};

void YXT_Sampling_Handler(void) {
    // 硬件反相适配 (根据逻辑分析仪相反波形)
    bool cur_level = !std_gpio_get_input_pin(GPIO_YXT_PORT, GPIO_YXT_PIN);
    
    if (++s_yxt_core.timeout_cnt > 500) {
        s_yxt_core.step = 0;
        s_yxt_core.timeout_cnt = 0;
    }

    if (cur_level == s_yxt_core.old_level) {
        if (cur_level) s_yxt_core.high_cnt++;
        else s_yxt_core.low_cnt++;
    } else {
        s_yxt_core.timeout_cnt = 0; 
        
        if (!cur_level) { 
            /* 下降沿：Bit 高电平结束 */
            
            // 修正：接收 105 位 (1起始位 + 104位数据)
            if (s_yxt_core.step >= 1 && s_yxt_core.step <= 105) {
                // 跳过第 1 位(起始位)，从第 2 位开始存入 DATA0
                if (s_yxt_core.step >= 2) {
                    uint8_t byte_idx = (s_yxt_core.step - 2) / 8;
                    // 位序 MSB：bit 7 -> bit 0 (解决 8 变 4 的偏移问题)
                    uint8_t bit_idx = 7 - ((s_yxt_core.step - 2) % 8); 
                    
                    if (s_yxt_core.high_cnt > s_yxt_core.low_cnt) {
                        s_yxt_core.buf[byte_idx] |= (1 << bit_idx);
                    } else {
                        s_yxt_core.buf[byte_idx] &= ~(1 << bit_idx);
                    }
                }
                
                // 收到第 105 位，标志 13 字节全收齐
                if (s_yxt_core.step == 105) s_yxt_core.frame_ready = true;
                s_yxt_core.step++;
            }
            s_yxt_core.low_cnt = 0;
        } else { 
            /* 上升沿：同步信号结束 */
            if (s_yxt_core.low_cnt > 200) {
                s_yxt_core.step = 1; 
                for(uint8_t i=0; i<13; i++) s_yxt_core.buf[i] = 0;
            }
            s_yxt_core.high_cnt = 0;
        }
        s_yxt_core.old_level = cur_level;
    }
}

/**
 * @brief 一线通数据解析任务 (建议 10ms - 20ms 调用一次)
 * 适用协议：台铃 Y34B (13 字节版)
 */
void YXT_Decode_Task(void) {
    // 1. 检查一帧数据是否接收完成
    if (!s_yxt_core.frame_ready) return;
    s_yxt_core.frame_ready = false; // 必须清除标志位

    // 2. 帧头二次验证 (防止杂波干扰)
    if (s_yxt_core.buf[0] != 0x08) return;

    // 3. 异或校验验证 (DATA0 ^ ... ^ DATA11 == DATA12)
    uint8_t check_val = 0;
    for (int i = 0; i < 12; i++) {
        check_val ^= s_yxt_core.buf[i];
    }
    if (check_val != s_yxt_core.buf[12]) return;

// 2. 速度解析逻辑
    // 判断 DATA5_D6 是否为 1 (1: 实际车速模式, 0: 霍尔脉冲模式)
    uint16_t raw_val = (uint16_t)((s_yxt_core.buf[7] << 8) | s_yxt_core.buf[8]);
    
    if (s_yxt_core.buf[5] & 0x40) { 
        // 模式 A：控制器直发速度 (实际车速 * 10) 
        g_yxt_data.speed = (uint8_t)(raw_val / 10);
    } else { 
        // 模式 B：霍尔脉冲模式，应用你的轮径补偿公式
        if (C_YXT_SPEED > 0) {
            g_yxt_data.speed = (uint8_t)(raw_val / (C_YXT_SPEED + R_YXT_SPEED_SET - 9));
        }
    }

// 3. SOC 电量格划分逻辑 [cite: 3, 37]
    uint8_t soc_percent = s_yxt_core.buf[9] & 0x7F; // 提取 0-100% 的数值
    
    if (soc_percent == 0) {
        g_yxt_data.soc = 0; // 0格 (通常对应闪烁)
    } else if (soc_percent <= SOC_LEVEL_1_LIMIT) {
        g_yxt_data.soc = 1; // 1格
    } else if (soc_percent <= SOC_LEVEL_2_LIMIT) {
        g_yxt_data.soc = 2; // 2格
    } else if (soc_percent <= SOC_LEVEL_3_LIMIT) {
        g_yxt_data.soc = 3; // 3格
    } else if (soc_percent <= SOC_LEVEL_4_LIMIT) {
        g_yxt_data.soc = 4; // 4格
    } else {
        g_yxt_data.soc = 5; // 5格 (满电)
    }

    // P档状态：DATA2 的 D1 位
    g_yxt_data.is_parking = (s_yxt_core.buf[2] >> 1) & 0x01;

    // 通讯在线标志
    g_yxt_data.is_online = true;
}
