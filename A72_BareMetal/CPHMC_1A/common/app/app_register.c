/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       app_register.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      Application task registration and software scheduling support.
 *@par        History
 *Date        Version   Author     Description
 *2026.09.11  1.0       zhaoguodong Initial version
 ******************************************************************************/

#include "app_register.h"
#include "handle_int_prog.h"

APP_TASK_CFG_STRUCT g_appTaskCfg;
APP_TASK_RT_STRUCT g_appLoopTaskRt[APP_MAX_LOOP_TASK_NUMBER];
APP_TASK_RT_STRUCT g_appIntrTaskRt[APP_MAX_INTR_TASK_NUMBER];

static HwiP_Handle g_appIntrHandle[APP_MAX_INTR_TASK_NUMBER];
static const uint32_t g_appIntrNumber[APP_MAX_INTR_TASK_NUMBER] =
{
    2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U
};

uint8_t app_register_loop_task(app_task_callback task_prog,
                               uint32_t task_period_us,
                               uint32_t task_timeout_us)
{
    uint32_t index;

    if ((task_prog == (app_task_callback)0) ||
        (task_period_us % APP_SOFT_INT_TIME_US != 0U) ||
        (task_timeout_us % APP_SOFT_INT_TIME_US != 0U))
    {
        return 0U;
    }

    for (index = 0U; index < APP_MAX_LOOP_TASK_NUMBER; index++)
    {
        if (g_appTaskCfg.loop_task_def[index].cfg_flag == 0U)
        {
            break;
        }
    }
    if (index >= APP_MAX_LOOP_TASK_NUMBER)
    {
        return 0U;
    }

    g_appTaskCfg.loop_task_def[index].cfg_flag = 1U;
    g_appTaskCfg.loop_task_def[index].task_period_time_us = task_period_us;
    g_appTaskCfg.loop_task_def[index].task_max_timeout_us = task_timeout_us;
    g_appTaskCfg.loop_task_def[index].p_task_prog = task_prog;
    g_appTaskCfg.loop_task_number++;
    g_appLoopTaskRt[index].p_task_prog = (void *)task_prog;
    return 1U;
}

uint8_t app_register_intr_task(app_task_callback task_prog,
                               uint32_t task_pri,
                               uint32_t task_period_us)
{
    uint32_t index;

    if ((task_prog == (app_task_callback)0) || (task_period_us == 0U) ||
        (task_pri < 3U) || (task_pri > 7U) ||
        (task_period_us % APP_SOFT_INT_TIME_US != 0U))
    {
        return 0U;
    }

    for (index = 0U; index < APP_MAX_INTR_TASK_NUMBER; index++)
    {
        if (g_appTaskCfg.intr_task_def[index].cfg_flag == 0U)
        {
            break;
        }
    }
    if (index >= APP_MAX_INTR_TASK_NUMBER)
    {
        return 0U;
    }

    g_appTaskCfg.intr_task_def[index].cfg_flag = 1U;
    g_appTaskCfg.intr_task_def[index].task_pri = task_pri;
    g_appTaskCfg.intr_task_def[index].task_period_time_us = task_period_us;
    g_appTaskCfg.intr_task_def[index].p_task_prog = task_prog;
    g_appTaskCfg.intr_task_number++;
    g_appIntrTaskRt[index].p_task_prog = (void *)task_prog;
    return 1U;
}

int32_t app_register_intr(void)
{
    uint32_t index;

    for (index = 0U; index < g_appTaskCfg.intr_task_number; index++)
    {
        HwiP_Params params;
        HwiP_Params_init(&params);
        params.priority = g_appTaskCfg.intr_task_def[index].task_pri;
        params.enableIntr = 1U;
        params.triggerSensitivity = OSAL_ARM_GIC_TRIG_TYPE_EDGE;
        g_appIntrHandle[index] = HwiP_create(g_appIntrNumber[index],
                                              g_appTaskCfg.intr_task_def[index].p_task_prog,
                                              &params);
        if (g_appIntrHandle[index] == (HwiP_Handle)0)
        {
            return -1;
        }
    }
    return 0;
}

void app_run_loop_tasks(void)
{
    uint32_t index;

    for (index = 0U; index < g_appTaskCfg.loop_task_number; index++)
    {
        if ((g_appLoopTaskRt[index].task_time * APP_SOFT_INT_TIME_US) >=
            g_appTaskCfg.loop_task_def[index].task_period_time_us)
        {
            g_appTaskCfg.loop_task_def[index].p_task_prog(0U);
            g_appLoopTaskRt[index].task_time = 0U;
        }
    }
}
