#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <stdint.h>

/**
 * @brief 任务结构体
 */
typedef struct {
    void (*task_func)(void);  // 任务函数指针
    uint32_t interval_ms;     // 运行间隔时间 (ms)
    uint32_t last_run_time;   // 上次运行的时间戳
} Task_t;

/* 函数声明 */
void Scheduler_Init(void);
void Scheduler_Run(void);

#endif
