/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       handle_int_prog.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      Hard-interrupt and software-interrupt dispatch implementation.
 *@par        History
 *Date        Version   Author     Description
 *2026.09.11  1.0       zhaoguodong Initial version
 ******************************************************************************/

#include "handle_int_prog.h"

#include <ti/osal/HwiP.h>

#include "app_register.h"
#include "bsp/interrupt/interrupt_ctrl.h"

void handle_hard_int_prog(uintptr_t arg)
{
    (void)arg;
    interrupt_ctrl_reload_timer();
    (void)HwiP_post(INTERRUPT_CTRL_SOFT_IRQ);
}

void handle_soft_int_prog(uintptr_t arg)
{
    uint32_t index;

    (void)arg;
    for (index = 0U; index < g_appTaskCfg.loop_task_number; index++)
    {
        g_appLoopTaskRt[index].task_time++;
    }
    for (index = 0U; index < g_appTaskCfg.intr_task_number; index++)
    {
        g_appIntrTaskRt[index].task_time++;
        if ((g_appIntrTaskRt[index].task_time * APP_SOFT_INT_TIME_US) >=
            g_appTaskCfg.intr_task_def[index].task_period_time_us)
        {
            (void)HwiP_post(2U + index);
            g_appIntrTaskRt[index].task_time = 0U;
        }
    }
}
