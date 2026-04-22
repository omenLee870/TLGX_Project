#include "yxt_proto.h"
#include "bsp_gpio.h"

/* 私有数据：相比原代码，我们增加了一个“超时保护”来重置状态 */
static struct {
    uint8_t  step;          // 当前接收到第几个 bit
    uint16_t low_cnt;       // 低电平计数
    uint16_t high_cnt;      // 高电平计数
    uint16_t timeout_cnt;   // 超时计数器
    uint8_t  buf[13];       // 完整存储 13 字节
    bool     frame_ready;   
    bool     old_level;     // 上一次的电平
} s_yxt_core = {0};

YXT_Data_t g_yxt_data = {0};

/**
 * @brief 核心解码：由 100us 中断调用
 * 优化点：增加了对 Y34B 协议 LSB 特性的支持，并加入同步码强制对齐
 */
void YXT_Sampling_Handler(void) {
    bool cur_level = std_gpio_get_input_pin(GPIO_YXT_PORT, GPIO_YXT_PIN);
    
    // 超时重置逻辑：如果 50ms (500 * 100us) 没收到跳变，重置 step，防止错位
    if (++s_yxt_core.timeout_cnt > 500) {
        s_yxt_core.step = 0;
        s_yxt_core.timeout_cnt = 0;
    }

    if (cur_level == s_yxt_core.old_level) {
        // 电平持续中
        if (cur_level) s_yxt_core.high_cnt++;
        else s_yxt_core.low_cnt++;
    } else {
        // 电平跳变发生
        s_yxt_core.timeout_cnt = 0; // 清除超时计时
        
        if (!cur_level) { 
            /* 下降沿触发：代表一个 Bit 的高电平部分结束了 */
            if (s_yxt_core.step > 0 && s_yxt_core.step <= 104) {
                uint8_t byte_idx = (s_yxt_core.step - 1) / 8;
                uint8_t bit_idx = (s_yxt_core.step - 1) % 8;
                
                // 学习原代码的精华：比例判定法
                if (s_yxt_core.high_cnt > s_yxt_core.low_cnt) {
                    s_yxt_core.buf[byte_idx] |= (1 << bit_idx);  // 逻辑 1 (LSB方式)
                } else {
                    s_yxt_core.buf[byte_idx] &= ~(1 << bit_idx); // 逻辑 0
                }
                
                if (s_yxt_core.step == 104) s_yxt_core.frame_ready = true;
                s_yxt_core.step++;
            }
            s_yxt_core.low_cnt = 0;
        } else { 
            /* 上升沿触发：代表低电平部分结束了 */
            // 同步码判定：长低电平 (>992 Tosc)
            // 在 100us 采样下，如果低电平超过 200 (20ms)，判定为帧起始
            if (s_yxt_core.low_cnt > 200) {
                s_yxt_core.step = 1; // 强制同步
                for(uint8_t i=0; i<13; i++) s_yxt_core.buf[i] = 0;
            }
            s_yxt_core.high_cnt = 0;
        }
        s_yxt_core.old_level = cur_level;
    }
}

/**
 * @brief 数据处理：由调度器任务调用 (50ms)
 * 优化点：严格按照台铃 Y34B 协议进行异或校验与解析
 */
void YXT_Decode_Task(void) {
    if (!s_yxt_core.frame_ready) return;
    s_yxt_core.frame_ready = false;

    // 1. 异或校验 (DATA0 ^ ... ^ DATA11 == DATA12)
    uint8_t check_val = 0;
    for (int i = 0; i < 12; i++) check_val ^= s_yxt_core.buf[i];
    
    if (check_val != s_yxt_core.buf[12]) return; // 校验失败丢弃

    // 2. 映射到全局结构体
    g_yxt_data.speed = s_yxt_core.buf[7];        // DATA7
    g_yxt_data.soc   = s_yxt_core.buf[9] & 0x7F; // DATA9
    g_yxt_data.is_parking = (s_yxt_core.buf[2] >> 1) & 0x01; // DATA2 D1
    g_yxt_data.is_online = true;
}
