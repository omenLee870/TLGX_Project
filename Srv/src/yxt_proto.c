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

/* --- 滤波与消抖宏参数 (可根据实际手感微调) --- */
#define SPEED_FILTER_ALPHA    3    // 速度滤波系数 (1-7，值越小越平滑但响应越慢，建议3)
#define SOC_FILTER_COUNT      15   // 电量消抖帧数 (连续收到15帧相同的电量等级才跳变

static struct {
    uint8_t  step;
    uint16_t low_cnt;
    uint16_t high_cnt;
    uint16_t timeout_cnt;
    uint8_t  buf[13];       // 严格存储 13 字节 (DATA0 - DATA12)
    bool     frame_ready;
    bool     old_level;

    // --- 新增滤波变量 ---
    uint16_t speed_filtered_x10;   // 内部维持 10 倍精度的速度缓存
    uint8_t  soc_confirm_lvl;      // 正在确认中的目标电量等级
    uint8_t  soc_confirm_cnt;      // 电量等级确认计数器
} s_yxt_core = {0};

YXT_Data_t g_yxt_data = {0};

/**
 * @brief 速度一阶低通滤波算法
 * 核心公式：$V_{out} = \frac{V_{out} \times (8 - \alpha) + V_{new} \times \alpha}{8}$
 */
static uint8_t Apply_Speed_Filter(uint16_t new_speed_x10) {
    // 采用位移代替除法提高效率
    s_yxt_core.speed_filtered_x10 = (s_yxt_core.speed_filtered_x10 * (8 - SPEED_FILTER_ALPHA) 
                                    + new_speed_x10 * SPEED_FILTER_ALPHA) >> 3;
    
    return (uint8_t)(s_yxt_core.speed_filtered_x10 / 10);
}

/**
 * @brief 电量等级消抖算法
 */
static uint8_t Apply_SOC_Filter(uint8_t raw_soc_percent) {
    uint8_t target_lvl = 0;

    // 1. 区间映射 (0-5级)
    if (raw_soc_percent == 0) target_lvl = 0;
    else if (raw_soc_percent <= 20)  target_lvl = 1;
    else if (raw_soc_percent <= 40)  target_lvl = 2;
    else if (raw_soc_percent <= 60)  target_lvl = 3;
    else if (raw_soc_percent <= 80)  target_lvl = 4;
    else target_lvl = 5;

    // 2. 消抖处理：只有连续多次收到新等级，才更新全局变量
    if (target_lvl != g_yxt_data.soc) {
        if (target_lvl == s_yxt_core.soc_confirm_lvl) {
            if (++s_yxt_core.soc_confirm_cnt >= SOC_FILTER_COUNT) {
                g_yxt_data.soc = target_lvl;
                s_yxt_core.soc_confirm_cnt = 0;
            }
        } else {
            s_yxt_core.soc_confirm_lvl = target_lvl;
            s_yxt_core.soc_confirm_cnt = 0;
        }
    } else {
        s_yxt_core.soc_confirm_cnt = 0;
    }
    return g_yxt_data.soc;
}

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
    
uint16_t speed_x10;

    if (s_yxt_core.buf[5] & 0x40) { // D6=1: 实际速度模式
        speed_x10 = raw_val; 
    } else { // D6=0: 霍尔脉冲模式
        speed_x10 = (C_YXT_SPEED > 0) ? (raw_val * 10 / C_YXT_SPEED) : 0;
    }
    g_yxt_data.speed = Apply_Speed_Filter(speed_x10);

    // 2. 电量滤波处理
    Apply_SOC_Filter(s_yxt_core.buf[9] & 0x7F);

    // P档状态：DATA2 的 D1 位
    g_yxt_data.is_parking = (s_yxt_core.buf[2] >> 1) & 0x01;

    // 通讯在线标志
    g_yxt_data.is_online = true;
}
