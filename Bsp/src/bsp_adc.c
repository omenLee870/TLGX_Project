#include "bsp_adc.h"
#include "bsp_gpio.h"

/* 校准相关宏定义，移植自原厂 demo */
#define CALFACT_MAX                            (31)
#define CALFACT_MIN                            (-31)
#define ADC_CALFACT_SYMBOL                     (ADC_CALFACT_CALFACT_5)
#define ADC_COMPENSATION_VALUE                 (*(int32_t *)(0x1FFF03CC))    

#define TIM_ARR_VALUE             (11718)   /* 计数时钟源为11.7K时，计时1s */  
#define TIM_PSC_VALUE             (12)      /* 系统时钟是48M，分频后时钟为11.7K */ 

/* ADC参考电压为VDDA：3300mV。在实际应用中需根据真实的VDD电压值设置该值 */
#define VREF_ADC_VDDA_VOLTAGE        (5000U)

volatile uint32_t g_voltage = 0;

/**
* @brief  提升ADC校准系数的精度 (移植自原厂 demo)
*/
void BSP_ADC_Software_Calibrate(void)
{
    int32_t get_calfact = 0;
    
    /* 使能校准 */
    std_adc_calibration_enable();
    
    /* 等待校准完成 */
    while(std_adc_get_flag(ADC_FLAG_EOCAL) == 0U);
    
    /* 清除ADC转换状态，确保之前状态不影响转换 */
    std_adc_clear_flag(ADC_FLAG_ALL);
    
    get_calfact = std_adc_get_calibration_factor();
    
    /* 判断校准系数符号位 */
    if(get_calfact & ADC_CALFACT_SYMBOL)
    {
        /* 校准系数是负值，转换成32位有符号负数，方便计算 */
        get_calfact = get_calfact | 0xFFFFFFE0;
    }
    
    /* 校准系数减去ADC补偿值获取新的校准系数 */
    get_calfact = get_calfact - ADC_COMPENSATION_VALUE;
    
    /* 判断校准系数是否超限 */
    if(get_calfact > CALFACT_MAX)
    {
        get_calfact = CALFACT_MAX;
    }
    else if(get_calfact < CALFACT_MIN)
    {
        get_calfact = CALFACT_MIN;
    }
    std_adc_calibration_factor_config(get_calfact);
}

/**
* @brief  ADC初始化
*/
void BSP_ADC_Init(void)
{
    /* TIM3初始化 */
    tim3_init();

    /* 1. 使能 ADC 时钟 */
    std_rcc_apb2_clk_enable(RCC_PERIPH_CLK_ADC);
       
    /* 2. ADC 时钟配置 (3分频) */
    std_adc_clock_config(ADC_CK_DIV3);
    
    /* TIM3 TRGO触发，上升沿有效 */
    std_adc_trig_config(ADC_TRIG_HW_EDGE_RISING,ADC_EXTRIG_TIM3_TRGO);
    
    /* 4. 循环间断转换模式 */
    std_adc_conversion_mode_config(ADC_DISCONTINUOUS_CONVER_MODE);
    
    /* 5. 采样时间配置 */
    std_adc_sampt_time_config(ADC_SAMPTIME_119CYCLES);
    
    /* 6. 选择通道 (PA4) */
    std_adc_fix_sequence_channel_enable(ADC_LIGHT_CH);

    /* 7. 配置wait模式，避免数据未及时读取，转换溢出 */
    std_adc_wait_mode_enable();
       
    /* 使能ADC */
    std_adc_enable();

    /* 等待ADC使能状态稳定 */
    std_delayus(ADC_EN_DELAY);

    /* 提升ADC校准系数精度，系统复位后只需执行一次 */
    BSP_ADC_Software_Calibrate();

    /* 使能TIM3 */
    std_tim_enable(TIM3);

    /* 启动转换 */
    std_adc_start_conversion();
}

/**
* @brief  读取一次 ADC 值 (软件触发方式)
*/
uint16_t BSP_ADC_ReadValue(void)
{
    /* 手动启动转换 */
    std_adc_start_conversion();
    
    /* 等待转换完成标志 EOC */
    while(std_adc_get_flag(ADC_FLAG_EOC) == 0U);
    
    /* 获取转换结果 */
    uint16_t res = std_adc_get_conversion_value();
    
    /* 清除标志位 */
    std_adc_clear_flag(ADC_FLAG_EOC);
    
    return res;
}


/**
* @brief  tim3初始化
* @retval 无
*/
void tim3_init(void)
{
    std_tim_basic_init_t basic_init_struct = {0};
    
    /* TIM3时钟使能 */
    std_rcc_apb1_clk_enable(RCC_PERIPH_CLK_TIM3);
            
    /* 配置TIM3计数器参数 */
    basic_init_struct.prescaler = TIM_PSC_VALUE;
    basic_init_struct.period = TIM_ARR_VALUE;
    basic_init_struct.clock_div = TIM_CLOCK_DTS_DIV1;
    std_tim_init(TIM3, &basic_init_struct);

    /* 清除更新事件标志 */
    std_tim_clear_flag(TIM3, TIM_FLAG_UPDATE);
    
    /* 配置tim3的TRGO信号源 */
    std_tim_trigout_mode_config(TIM3, TIM_TRIG_OUT_UPDATE);
}


uint32_t bsp_adc_get_adcvalue(void)
{
// 只有当 EOC (End of Conversion) 为 1 时，才说明当前转换完成了
    if (std_adc_get_flag(ADC_FLAG_EOC) != 0U) 
    {
        /* 获取采样值并转换为电压值 */
        g_voltage = std_adc_get_conversion_value() * VREF_ADC_VDDA_VOLTAGE / ADC_CONVER_SCALE;
        
        /* 按照 Demo，读取后清除 EOC 标志 */
        std_adc_clear_flag(ADC_FLAG_EOC);
    }

    // 只有当 EOS (End of Sequence) 为 1 时，才说明整个序列结束了
    if (std_adc_get_flag(ADC_FLAG_EOS) != 0U) 
    {
        /* 清除 EOS 标志 */
        std_adc_clear_flag(ADC_FLAG_EOS);
    }    
    
    // 返回最新的电压值（如果没有新转换，则返回上次保存的旧值）
    return g_voltage;
}

