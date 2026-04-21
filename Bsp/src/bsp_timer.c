#include "bsp_timer.h"
#include "yxt_proto.h"

// 全局系统 Tick (单位: 1ms)
volatile uint32_t sys_tick_1ms = 0;

void BSP_Timer_Init(void)
{
    std_tim_basic_init_t tim_init; 

    // 1. 开启 TIM1 时钟 (APB2总线)
    std_rcc_apb2_clk_enable(RCC_PERIPH_CLK_TIM1); 

    // 2. 配置时基参数 (48MHz 主频下 100us)
    tim_init.prescaler      = 48 - 1;               // 48分频 (计数频率 1MHz)
    tim_init.counter_mode   = TIM_COUNTER_MODE_UP;  
    tim_init.period         = 100 - 1;              // 100us 溢出一次
    tim_init.clock_div      = TIM_CLOCK_DTS_DIV1;   
    tim_init.repeat_counter = 0;

    std_tim_init(TIM1, &tim_init); 

    // 3. 使能更新中断 (UIE)
    std_tim_interrupt_enable(TIM1, TIM_INTERRUPT_UPDATE); 

    // 4. 配置 NVIC 
    NVIC_ClearPendingIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
    NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn, 0); // 设置为最高优先级
    NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);      // 

    // 5. 启动定时器
    std_tim_enable(TIM1); 
}

/**
 * @brief TIM1 复合中断服务入口
 * @note 对应 TIM1_BRK_UP_TRG_COM_IRQn 
 */
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    // 检查是否为“更新中断”标志 (UIF)
    if (std_tim_get_flag(TIM1, TIM_FLAG_UPDATE)) 
    {
        // 必须手动清除标志位
        std_tim_clear_flag(TIM1, TIM_FLAG_UPDATE); 

        // --- 1ms 系统 Tick 处理 ---
        static uint8_t ms_cnt = 0;
        if (++ms_cnt >= 10) 
        {
            ms_cnt = 0;
            sys_tick_1ms++;
        }

        // --- YXT 采样处理 ---
        YXT_Sampling_Handler();
    }
}
