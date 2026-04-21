#include "drv_sm16306.h"
#include "bsp_gpio.h"

// 极简延迟宏 (针对 48MHz)
#define SM_DELAY()  __NOP(); __NOP()

/**
 * @brief 向 SM16306 写入 16 位数据
 * @param data 对应 16 个通道的开关状态 (1:亮, 0:灭)
 */
void Drv_SM16306_Write(uint16_t data) {
    // 1. 串行移位输入数据
    for (int i = 0; i < 16; i++) {
        // 先准备数据 (SDI)
        if (data & 0x8000) {
            std_gpio_set_pin(GPIO_SM_PORT_B, PIN_SM_SDI);
        } else {
            std_gpio_reset_pin(GPIO_SM_PORT_B, PIN_SM_SDI);
        }
        data <<= 1;
        SM_DELAY();

        // 产生时钟上升沿 (CLK)
        std_gpio_set_pin(GPIO_SM_PORT_A, PIN_SM_CLK);
        SM_DELAY();
        std_gpio_reset_pin(GPIO_SM_PORT_A, PIN_SM_CLK);
    }

    // 2. 锁存数据到输出寄存器 (LE)
    // 根据 SM16306 特性，LE 高电平期间锁存器直通，下降沿锁存
    std_gpio_set_pin(GPIO_SM_PORT_A, PIN_SM_LE);
    SM_DELAY();
    std_gpio_reset_pin(GPIO_SM_PORT_A, PIN_SM_LE);
}

/**
 * @brief 控制显示开关
 * @param enable true: 开启显示, false: 关闭显示
 */
void Drv_SM16306_DisplayControl(bool enable) {
    if (enable) {
        std_gpio_reset_pin(GPIO_SM_PORT_A, PIN_SM_OE); // OE 低电平有效 [cite: 14, 21]
    } else {
        std_gpio_set_pin(GPIO_SM_PORT_A, PIN_SM_OE);
    }
}
