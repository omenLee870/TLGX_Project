#include "app_ui.h"
#include "yxt_proto.h"
#include "drv_sm16306.h"
#include "bsp_gpio.h"

/* --- 硬件 IO 映射 --- */
#define T_SEG_A  (1U << 8)
#define T_SEG_B  (1U << 13)
#define T_SEG_C  (1U << 12)
#define T_SEG_D  (1U << 11)
#define T_SEG_E  (1U << 10)
#define T_SEG_F  (1U << 9)
#define T_SEG_G  (1U << 14)

#define U_SEG_A  (1U << 0)
#define U_SEG_B  (1U << 5)
#define U_SEG_C  (1U << 4)
#define U_SEG_D  (1U << 3)
#define U_SEG_E  (1U << 2)
#define U_SEG_F  (1U << 1)
#define U_SEG_G  (1U << 6)

#define BATT_BAR1 ((1U << 13) | (1U << 14))
#define BATT_BAR2 ((1U << 11) | (1U << 12))
#define BATT_BAR3 ((1U << 9)  | (1U << 10))
#define BATT_BAR4 ((1U << 3)  | (1U << 6))
#define BATT_BAR5 ((1U << 2)  | (1U << 4))

#define ICON_OIL_CAN  (1U << 8)
#define ICON_KMH      (1U << 1)
#define ICON_RESERVED (1U << 0)

static const uint16_t TENS_DIGIT_TABLE[] = {
    T_SEG_A | T_SEG_B | T_SEG_C | T_SEG_D | T_SEG_E | T_SEG_F,
    T_SEG_B | T_SEG_C,
    T_SEG_A | T_SEG_B | T_SEG_G | T_SEG_E | T_SEG_D,
    T_SEG_A | T_SEG_B | T_SEG_G | T_SEG_C | T_SEG_D,
    T_SEG_F | T_SEG_G | T_SEG_B | T_SEG_C,
    T_SEG_A | T_SEG_F | T_SEG_G | T_SEG_C | T_SEG_D,
    T_SEG_A | T_SEG_F | T_SEG_E | T_SEG_D | T_SEG_C | T_SEG_G,
    T_SEG_A | T_SEG_B | T_SEG_C,
    T_SEG_A | T_SEG_B | T_SEG_C | T_SEG_D | T_SEG_E | T_SEG_F | T_SEG_G,
    T_SEG_A | T_SEG_B | T_SEG_C | T_SEG_D | T_SEG_F | T_SEG_G
};

static const uint16_t UNITS_DIGIT_TABLE[] = {
    U_SEG_A | U_SEG_B | U_SEG_C | U_SEG_D | U_SEG_E | U_SEG_F,
    U_SEG_B | U_SEG_C,
    U_SEG_A | U_SEG_B | U_SEG_G | U_SEG_E | U_SEG_D,
    U_SEG_A | U_SEG_B | U_SEG_G | U_SEG_C | U_SEG_D,
    U_SEG_F | U_SEG_G | U_SEG_B | U_SEG_C,
    U_SEG_A | U_SEG_F | U_SEG_G | U_SEG_C | U_SEG_D,
    U_SEG_A | U_SEG_F | U_SEG_E | U_SEG_D | U_SEG_C | U_SEG_G,
    U_SEG_A | U_SEG_B | U_SEG_C,
    U_SEG_A | U_SEG_B | U_SEG_C | U_SEG_D | U_SEG_E | U_SEG_F | U_SEG_G,
    U_SEG_A | U_SEG_B | U_SEG_C | U_SEG_D | U_SEG_F | U_SEG_G
};

static uint16_t g_display_buffer[NUM_SCAN_GROUPS];// 显示缓存
static bool s_ui_is_inited = false;

void App_UI_Init(void) {
    for(int i = 0; i < NUM_SCAN_GROUPS; i++) g_display_buffer[i] = 0;
    s_ui_is_inited = true;
}
/**
 * @brief 测试版逻辑任务：自动变动数字并闪烁图标
 * @note 假设调度器频率为 100ms
 */
void App_UI_Logic_Task(void) {
    // 1. 防御性懒初始化
    if (!s_ui_is_inited) {
        App_UI_Init();
        return;
    }

    // 2. 演示逻辑计时器
    static uint16_t test_counter = 0;
    static uint8_t  disp_speed = 0; 
    static bool     blink_toggle = false;

    test_counter++;

    // 500ms 周期翻转闪烁状态 (5 * 100ms)
    if (test_counter % 5 == 0) blink_toggle = !blink_toggle;

    // 1000ms 周期速度加 1 (10 * 100ms)
    if (test_counter >= 10) {
        test_counter = 0;
        if (++disp_speed > 99) disp_speed = 0;
    }

    // 3. 构建显示位图
    uint16_t tens_mask = (disp_speed >= 10) ? TENS_DIGIT_TABLE[disp_speed / 10] : 0;
    uint16_t units_mask = UNITS_DIGIT_TABLE[disp_speed % 10];

    // DA, DB 组：显示数字 (双 LED 冗余映射) [cite: 4]
    g_display_buffer[0] = tens_mask | units_mask; 
    g_display_buffer[1] = tens_mask | units_mask; 

    // DC 组：测试全图标闪烁效果
    if (blink_toggle) {
        g_display_buffer[2] = 0xFFFF; // 全亮测试
    } else {
        g_display_buffer[2] = 0;      // 全灭
    }
}

/**
 * @brief 物理扫描刷新 (由调度器 4ms 调用一次)
 */
void Task_DisplayUpdate(void) {
    // 关键防御：如果显示系统未就绪，强制消隐并退出
    if (!s_ui_is_inited) {
        std_gpio_set_pin(GPIOB, GPIO_SCAN_A_PIN | GPIO_SCAN_B_PIN | GPIO_SCAN_C_PIN);
        return;
    }

    static uint8_t idx = 0;
    // 1. 消隐 
    std_gpio_set_pin(GPIOB, GPIO_SCAN_A_PIN | GPIO_SCAN_B_PIN | GPIO_SCAN_C_PIN);
    // 2. 数据下发 [cite: 1]
    Drv_SM16306_Write(g_display_buffer[idx]);
    // 3. 选通对应 COM 组 (PB1-PB3) [cite: 4]
    switch(idx) {
        case 0: std_gpio_reset_pin(GPIOB, GPIO_SCAN_A_PIN); break;
        case 1: std_gpio_reset_pin(GPIOB, GPIO_SCAN_B_PIN); break;
        case 2: std_gpio_reset_pin(GPIOB, GPIO_SCAN_C_PIN); break;
    }
    if (++idx >= NUM_SCAN_GROUPS) idx = 0;
}
