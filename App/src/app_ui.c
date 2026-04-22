#include "app_ui.h"
#include "yxt_proto.h"
#include "drv_sm16306.h"
#include "bsp_gpio.h"

/* --- 硬件 IO 映射 --- */
#define T_SEG_A  (1U << 8)
#define T_SEG_B  (1U << 9)
#define T_SEG_C  (1U << 10)
#define T_SEG_D  (1U << 11)
#define T_SEG_E  (1U << 12)
#define T_SEG_F  (1U << 13)
#define T_SEG_G  (1U << 14)

#define U_SEG_A  (1U << 0)
#define U_SEG_B  (1U << 1)
#define U_SEG_C  (1U << 2)
#define U_SEG_D  (1U << 3)
#define U_SEG_E  (1U << 4)
#define U_SEG_F  (1U << 5)
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
    if (s_ui_is_inited) return; // 避免重复初始化

    /* 1. 硬件驱动层配置 */
    Drv_SM16306_Write(0x0000);         // 初始清零
    Drv_SM16306_DisplayControl(true);  // 开启显示控制

    /* 2. 应用业务层配置 */
    for(int i = 0; i < NUM_SCAN_GROUPS; i++) {
        g_display_buffer[i] = 0;       // 清空显示缓存
    }

    s_ui_is_inited = true;             // 标记已完成初始化
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

   // 2. 获取一线通解码后的速度数据
    // 这里的 g_yxt_data.speed 是由 YXT_Decode_Task 解析出来的真实数据
    uint8_t current_speed = g_yxt_data.speed;

    // 3. 构建显示位图
    // 限制最高显示速度为 99，防止超出段码表索引
    if (current_speed > 99) current_speed = 99;

    // 获取十位数和个位数的段码掩码
    uint16_t tens_mask = (current_speed >= 10) ? TENS_DIGIT_TABLE[current_speed / 10] : 0;
    uint16_t units_mask = UNITS_DIGIT_TABLE[current_speed % 10];

    // 4. 将位图写入显示缓存
    // 根据你的硬件定义，g_display_buffer[0] 和 [1] 负责数字显示
    g_display_buffer[0] = tens_mask | units_mask; 
    g_display_buffer[1] = tens_mask | units_mask; 

    /* --- 第二部分：电量与图标显示 (DC 组) --- */
    uint16_t icon_buffer = ICON_KMH; // 默认 KM/H 单位图标常亮
    
    // 使用静态变量作为简单的闪烁计时器 (假设任务周期 50ms，10 次为 500ms)
    static uint8_t ui_blink_cnt = 0;
    ui_blink_cnt++;

    // 根据 soc 等级 (0-5) 点亮电量格
    switch (g_yxt_data.soc) {
        case 5: 
            icon_buffer |= BATT_BAR5; // 满电，点亮全部格
        case 4: 
            icon_buffer |= BATT_BAR4;
        case 3: 
            icon_buffer |= BATT_BAR3;
        case 2: 
            icon_buffer |= BATT_BAR2;
        case 1: 
            icon_buffer |= BATT_BAR1; // 1 格电
            break;

        case 0:
            // 情况 A：SOC 为 0 油壶灯 同步闪烁 (频率约 1Hz)
            if ((ui_blink_cnt % 20) < 10) { // 前 500ms 亮，后 500ms 灭
                icon_buffer |= ICON_OIL_CAN;
            }

            break;

        default:
            break;
    }

    // P 档图标逻辑 (如果 g_yxt_data.is_parking 为真)
    if (g_yxt_data.is_parking) {
        // 假设你有一个 P 档图标宏，可以加在这里
        // icon_buffer |= ICON_PARKING; 
    }

    // 将最终生成的图标位图存入 DC 扫描组
    g_display_buffer[2] = icon_buffer;
}

/**
 * @brief 物理扫描刷新 (由调度器 4ms 调用一次)
 */
void Task_DisplayUpdate(void) {
    // 关键防御：如果显示系统未就绪，强制消隐并退出
    if (!s_ui_is_inited) {
        App_UI_Init();
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
