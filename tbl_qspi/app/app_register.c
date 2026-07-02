/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       handle_int_prog.h
 *@author     jinyangh
 *@date       2025.10.21
 *@brief      Provide a registration task interface.
 *@par        History
 *Date        Version   Author     Description
 *2025.06.13  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include "app_register.h"
#include "handle_int_prog.h"
#include "ti/osal/RegisterIntr.h"
#include "ti/osal/DebugP.h"
#include "ti/csl/soc/j721e/src/cslr_intr_r5fss1.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
ALL_TASK_DEF_CFG_STRUCT task_cfg = {0};
TASK_RT_INF_STRUCT task_loop_rt[MAX_LOOP_TASK_NUMBER] = {0};
TASK_RT_INF_STRUCT task_intr_rt[MAX_INTR_TASK_NUMBER] = {0};
uint32_t intr_num[MAX_INTR_TASK_NUMBER] = {
    CSLR_R5FSS1_CORE1_INTR_EHRPWM0_EPWM_ETINT_0,
    CSLR_R5FSS1_CORE1_INTR_EHRPWM1_EPWM_ETINT_0,
    CSLR_R5FSS1_CORE1_INTR_EHRPWM2_EPWM_ETINT_0,
    CSLR_R5FSS1_CORE1_INTR_EHRPWM3_EPWM_ETINT_0,
    CSLR_R5FSS1_CORE1_INTR_EHRPWM4_EPWM_ETINT_0,
    CSLR_R5FSS1_CORE1_INTR_EHRPWM5_EPWM_ETINT_0,
    CSLR_R5FSS1_CORE1_INTR_EHRPWM0_EPWM_TRIPZINT_0,
    CSLR_R5FSS1_CORE1_INTR_EHRPWM1_EPWM_TRIPZINT_0,
};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/**
 * @brief                  : 注册定时任务
 * @param  task_prog       : 任务的执行程序
 * @param  task_period_us  : 任务定时周期，如果是0，就是每次循环就执行（以驱动层的软中断为单位）
 * @param  task_timeout_us :
 * 任务最大超时时间，如果过了这个时间，任务还没执行，那说明任务到了规定时间未执行（以驱动层的软中断为单位）
 * @return                 : 1成功，0失败
 * @attention              : 修改应用层逻辑时不可修改此函数
 */
uint8_t
register_loop_task(app_task_callback task_prog, uint32_t task_period_us, uint32_t task_timeout_us)
{
    uint8_t index;

    if (task_prog == NULL)
    {
        return 0;
    }

    if (task_period_us % SOFT_INT_TIME_US || task_timeout_us % SOFT_INT_TIME_US)
    {
        return 0;
    }

    for (index = 0; index < MAX_LOOP_TASK_NUMBER; index++)
    {
        if (task_cfg.loop_task_def[index].cfg_flag == 0)
        {
            break;
        }
    }

    if (index >= MAX_LOOP_TASK_NUMBER)
    {
        return 0;
    }

    task_cfg.loop_task_def[index].cfg_flag = 1;
    task_cfg.loop_task_def[index].p_task_prog = task_prog;
    task_cfg.loop_task_def[index].task_period_time_us = task_period_us;
    task_cfg.loop_task_def[index].task_max_timeout_us = task_timeout_us;
    task_cfg.loop_task_number++;

    task_loop_rt[index].p_task_prog = (void *)task_prog;

    return 1;
}

/**
 * @brief                 : 注册中断
 * @param  task_prog      : 中断执行程序
 * @param  task_pri       : 中断优先级（允许的设置范围是3~13）
 * @param  task_period_us : 中断周期（以驱动层的软中断为单位）
 * @return                : 1成功，0失败
 * @attention             : 修改应用层逻辑时不可修改此函数
 */
uint8_t register_intr_task(app_task_callback task_prog, uint32_t task_pri, uint32_t task_period_us)
{
    uint8_t index;

    if (task_prog == NULL || task_period_us <= 0 || task_pri < 3 || task_pri > 13)
    {
        return 0;
    }

    if (task_period_us % SOFT_INT_TIME_US)
    {
        return 0;
    }

    for (index = 0; index < MAX_INTR_TASK_NUMBER; index++)
    {
        if (task_cfg.intr_task_def[index].cfg_flag == 0)
        {
            break;
        }
    }

    if (index >= MAX_INTR_TASK_NUMBER)
    {
        return 0;
    }

    task_cfg.intr_task_def[index].cfg_flag = 1;
    task_cfg.intr_task_def[index].p_task_prog = task_prog;
    task_cfg.intr_task_def[index].task_pri = task_pri;
    task_cfg.intr_task_def[index].task_period_time_us = task_period_us;
    task_cfg.intr_task_number++;

    task_intr_rt[index].p_task_prog = (void *)task_prog;

    return 1;
}

// 修改应用层逻辑时不可修改此函数
void app_register_intr(void)
{
    HwiP_Params hwiParams;
    HwiP_Handle hHwi;

    for (int i = 0; i < task_cfg.intr_task_number; i++)
    {
        HwiP_Params_init(&hwiParams);
        hwiParams.priority = task_cfg.intr_task_def[i].task_pri;
        hHwi = HwiP_create(intr_num[i], task_cfg.intr_task_def[i].p_task_prog, &hwiParams);
        DebugP_assert(hHwi != NULL);
    }
}