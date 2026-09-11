/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       app_register.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      Application loop-task and interrupt-task registration interface.
 *@par        History
 *Date        Version   Author     Description
 *2026.09.11  1.0       zhaoguodong Initial version
 ******************************************************************************/
#ifndef _APP_REGISTER_H
#define _APP_REGISTER_H

#include <stdint.h>

#include <ti/osal/HwiP.h>

#define APP_MAX_LOOP_TASK_NUMBER  (8U)
#define APP_MAX_INTR_TASK_NUMBER  (8U)

typedef void (*app_task_callback)(uintptr_t arg);

typedef struct
{
    uint32_t cfg_flag;
    uint32_t task_period_time_us;
    uint32_t task_max_timeout_us;
    uint32_t task_pri;
    app_task_callback p_task_prog;
} APP_TASK_DEF_STRUCT;

typedef struct
{
    uint8_t loop_task_number;
    uint8_t intr_task_number;
    APP_TASK_DEF_STRUCT loop_task_def[APP_MAX_LOOP_TASK_NUMBER];
    APP_TASK_DEF_STRUCT intr_task_def[APP_MAX_INTR_TASK_NUMBER];
} APP_TASK_CFG_STRUCT;

typedef struct
{
    void *p_task_prog;
    volatile uint32_t task_time;
    uint32_t max_task_cost_time_us;
} APP_TASK_RT_STRUCT;

extern APP_TASK_CFG_STRUCT g_appTaskCfg;
extern APP_TASK_RT_STRUCT g_appLoopTaskRt[APP_MAX_LOOP_TASK_NUMBER];
extern APP_TASK_RT_STRUCT g_appIntrTaskRt[APP_MAX_INTR_TASK_NUMBER];

uint8_t app_register_loop_task(app_task_callback task_prog,
                               uint32_t task_period_us,
                               uint32_t task_timeout_us);
uint8_t app_register_intr_task(app_task_callback task_prog,
                               uint32_t task_pri,
                               uint32_t task_period_us);
int32_t app_register_intr(void);
void app_run_loop_tasks(void);

#endif
