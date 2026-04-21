#ifndef __SYS_CONFIG_H
#define __SYS_CONFIG_H

/**
 * @brief 全局项目目标选择
 * 每次编译新项目时，只需在这里切换宏定义即可
 */

// #define PROJECT_TARGET_ALPHA    // 示例项目A：48V标准款
#define PROJECT_TARGET_DEFAULT     // 默认项目：对应原代码逻辑
// #define PROJECT_TARGET_ZONGSHEN // 宗申定制款

/* 根据选择的项目包含对应的参数文件 */
#if defined(PROJECT_TARGET_DEFAULT)
    #include "target_default.h"
#elif defined(PROJECT_TARGET_ZONGSHEN)
    #include "target_zongshen.h"
#else
    #error "No project target defined! Please check sys_config.h"
#endif

#endif
