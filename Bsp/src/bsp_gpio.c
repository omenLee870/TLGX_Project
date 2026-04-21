#include "bsp_gpio.h"

void BSP_GPIO_Init(void)
{
    std_gpio_init_t gpio_init;

    // 开启 GPIOA 和 GPIOB 时钟
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA | RCC_PERIPH_CLK_GPIOB);

    // 初始化LED组开关引脚 (PB1, PB2, PB3, PB4)
    gpio_init.pin         = GPIO_SCAN_A_PIN | GPIO_SCAN_B_PIN | GPIO_SCAN_C_PIN | GPIO_HEADLIGHT_PIN;
    gpio_init.mode        = GPIO_MODE_OUTPUT;       // 输出模式 [cite: 19]
    gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;  // 推挽输出 [cite: 19]
    gpio_init.pull        = GPIO_NOPULL;           // 不带上下拉 [cite: 19]
    std_gpio_init(GPIOB, &gpio_init);
    // 默认全部拉高（关闭 PMOS 供电）
    std_gpio_set_pin(GPIOB, gpio_init.pin);
    std_gpio_reset_pin(GPIOB, GPIO_HEADLIGHT_PIN);

    // 初始化 SM16306 相关引脚 (PA0, PA1, PA6)
    gpio_init.pin         = PIN_SM_CLK | PIN_SM_LE | PIN_SM_OE;
    gpio_init.mode        = GPIO_MODE_OUTPUT;
    gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;
    gpio_init.pull        = GPIO_NOPULL;
    std_gpio_init(GPIO_SM_PORT_A, &gpio_init);
    gpio_init.pin         = PIN_SM_SDI;
    std_gpio_init(GPIO_SM_PORT_B, &gpio_init);
    // 初始状态设置
    std_gpio_reset_pin(GPIO_SM_PORT_A, PIN_SM_CLK | PIN_SM_LE); // CLK/LE 低
    std_gpio_set_pin(GPIO_SM_PORT_A, PIN_SM_OE); // OE 高 (默认关闭输出)

    // 初始化一线通采样引脚 (PA7)
    gpio_init.pin         = GPIO_YXT_PIN;
    gpio_init.mode        = GPIO_MODE_INPUT;    // 输入模式 [cite: 19]
    gpio_init.pull        = GPIO_PULLUP;       // 开启上拉防止悬空干扰 [cite: 19]
    std_gpio_init(GPIOA, &gpio_init);

}
