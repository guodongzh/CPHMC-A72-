/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       handle_int_prog.h
*@author     LiuRui
*@date       2025.10.21
*@brief      Provide a registration task interface.
*@par        History
*Date        Version   Author     Description
*2025.06.13  1.0       jinyangh    example
******************************************************************************/
#ifndef _APP_REGISTER_H
#define _APP_REGISTER_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "ti/osal/RegisterIntr.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define MAX_LOOP_TASK_NUMBER 8
#define MAX_INTR_TASK_NUMBER 8
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef void (*app_task_callback)(uintptr_t arg);

typedef struct
{
    uint32_t cfg_flag;              // 已配置标志
    uint32_t task_period_time_us;   // 任务执行周期，以us为单位
    uint32_t task_max_timeout_us;   // 任务最大超时时间，以us为单位，如果过了这个时间，任务还没执行，那说明任务到了规定时间未执行
    uint32_t task_pri;              // 任务优先级
    app_task_callback p_task_prog;  // 任务函数
} EACH_TASK_DEF_STRUCT;

typedef struct
{
    uint8_t loop_task_number;  // 循环任务个数
    uint8_t intr_task_number;  // 中断任务个数
    EACH_TASK_DEF_STRUCT loop_task_def[MAX_LOOP_TASK_NUMBER];
    EACH_TASK_DEF_STRUCT intr_task_def[MAX_INTR_TASK_NUMBER];
    uint32_t task_sum;  // 整个结构体32bit校验和，用于定时检查
} ALL_TASK_DEF_CFG_STRUCT;

typedef struct
{
    void *p_task_prog;               // 指向注册的任务处理函数的指针，用于在任务处理函数中检测是否是自己
    uint32_t task_time;              // 任务调度计时器，以硬中断的执行周期为计数单位
    uint32_t max_task_cost_time_us;  // 本任务花费的最大时间us数，用于统计该任务花费时间，每2min清除一下，即置0
} TASK_RT_INF_STRUCT;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern ALL_TASK_DEF_CFG_STRUCT task_cfg;
extern TASK_RT_INF_STRUCT task_loop_rt[MAX_LOOP_TASK_NUMBER];
extern TASK_RT_INF_STRUCT task_intr_rt[MAX_INTR_TASK_NUMBER];
extern uint32_t intr_num[MAX_INTR_TASK_NUMBER];

uint8_t register_loop_task(app_task_callback task_prog, uint32_t task_period_us, uint32_t task_timeout_us);
uint8_t register_intr_task(app_task_callback task_prog, uint32_t task_pri, uint32_t task_period_us);
void app_register_intr(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _APP_REGISTER_H */
