#include "bsp_gpio.h"

void BSP_GPIO_Init(void)
{
std_gpio_init_t gpio_init;

    // 开启 GPIOA 和 GPIOB 时钟
    std_rcc_gpio_clk_enable(RCC_PERIPH_CLK_GPIOA | RCC_PERIPH_CLK_GPIOB);

    // 初始化 PA0, PA1, PA6 (CLK, LE, OE)
    gpio_init.pin         = PIN_SM_CLK | PIN_SM_LE | PIN_SM_OE;
    gpio_init.mode        = GPIO_MODE_OUTPUT;
    gpio_init.output_type = GPIO_OUTPUT_PUSHPULL;
    gpio_init.pull        = GPIO_NOPULL;
    std_gpio_init(GPIO_SM_PORT_A, &gpio_init);

    // 初始化 PB0 (SDI)
    gpio_init.pin         = PIN_SM_SDI;
    std_gpio_init(GPIO_SM_PORT_B, &gpio_init);

    // 初始状态设置
    std_gpio_reset_pin(GPIO_SM_PORT_A, PIN_SM_CLK | PIN_SM_LE); // CLK/LE 低
    std_gpio_set_pin(GPIO_SM_PORT_A, PIN_SM_OE); // OE 高 (默认关闭输出)
}
