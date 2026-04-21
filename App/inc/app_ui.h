#ifndef __APP_UI_H
#define __APP_UI_H

#include <stdint.h>

/* --- 硬件架构配置宏 --- */
#define NUM_SCAN_GROUPS    3      // 扫描组数量：DA, DB, DC

/**
 * @brief 初始化显示业务
 */
void App_UI_Init(void);

/**
 * @brief UI 逻辑刷新任务 (由调度器以较低频率调用，如 50ms)
 * 负责将解析到的数据转换为显存位图
 */
void App_UI_Logic_Task(void);

/**
 * @brief 硬件扫描任务 (由调度器以高频率调用，如 4ms)
 * 负责物理上的分时刷新点亮
 */
void Task_DisplayUpdate(void);

#endif
