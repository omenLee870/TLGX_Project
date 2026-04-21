#include "scheduler.h"
#include <stddef.h>

/* 外部引入系统毫秒计数 */
extern volatile uint32_t sys_tick_1ms;

/* ------------------------------------------------------------------
 * [任务注册区]
 * 以后新增功能（比如转向灯闪烁、电量计算），只需在这里加一行
 * ------------------------------------------------------------------ */
 
// 暂时放空函数作为占位符，后续我们会把具体的业务填进去
extern void YXT_Decode_Task(void);
void Task_WorkState(void);
void Task_DisplayUpdate(void);


static Task_t task_list[] = {
    {YXT_Decode_Task,       20,  0},// 20ms 解码一次数据包
    {Task_WorkState,     100, 0},  // 100ms 处理一次主逻辑状态机
    {Task_DisplayUpdate, 100, 0},  // 100ms 刷新一次屏幕显示
};

#define TASK_COUNT (sizeof(task_list) / sizeof(task_list[0]))

/**
 * @brief 调度器初始化
 */
void Scheduler_Init(void) {
    uint32_t current_time = sys_tick_1ms;
    for (int i = 0; i < TASK_COUNT; i++) {
        task_list[i].last_run_time = current_time;
    }
}

/**
 * @brief 调度器核心执行器 (放在 main 的 while(1) 中)
 */
void Scheduler_Run(void) {
    uint32_t current_time = sys_tick_1ms;

    for (int i = 0; i < TASK_COUNT; i++) {
        // 判断时间片是否到达 (考虑了 sys_tick_1ms 溢出的情况)
        if (current_time - task_list[i].last_run_time >= task_list[i].interval_ms) {
            task_list[i].last_run_time = current_time;
            
            if (task_list[i].task_func != NULL) {
                task_list[i].task_func(); // 执行任务
            }
        }
    }
}

/* 占位函数实现 (防止编译报错) */
void Task_WorkState(void) {}
void Task_DisplayUpdate(void) {}
