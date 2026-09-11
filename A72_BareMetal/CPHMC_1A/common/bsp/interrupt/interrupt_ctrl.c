/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       interrupt_ctrl.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 generic-timer hard-interrupt and soft-interrupt setup.
 *@par        History
 *Date        Version   Author     Description
 *2026.09.11  1.0       zhaoguodong Initial version
 ******************************************************************************/

#include "bsp/interrupt/interrupt_ctrl.h"

#include <ti/osal/HwiP.h>

#include "../../app/handle_int_prog.h"

static HwiP_Handle g_hardTimerHandle;
static HwiP_Handle g_softInterruptHandle;
static uint64_t g_timerReloadTicks;

static uint64_t interrupt_ctrl_read_frequency(void)
{
    uint64_t value;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(value));
    return value;
}

void interrupt_ctrl_reload_timer(void)
{
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r"(g_timerReloadTicks));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r"(1ULL));
    __asm__ volatile("isb");
}

int32_t interrupt_ctrl_init(void)
{
    HwiP_Params params;
    uint64_t frequency = interrupt_ctrl_read_frequency();

    g_timerReloadTicks = frequency / 1000000ULL * APP_SOFT_INT_TIME_US;
    if (g_timerReloadTicks == 0ULL)
    {
        return -1;
    }

    HwiP_Params_init(&params);
    params.priority = 2U;
    params.enableIntr = 1U;
    params.triggerSensitivity = OSAL_ARM_GIC_TRIG_TYPE_LEVEL;
    g_hardTimerHandle = HwiP_create(INTERRUPT_CTRL_HARD_IRQ,
                                    handle_hard_int_prog,
                                    &params);
    if (g_hardTimerHandle == (HwiP_Handle)0)
    {
        return -1;
    }

    HwiP_Params_init(&params);
    params.priority = 3U;
    params.enableIntr = 1U;
    params.triggerSensitivity = OSAL_ARM_GIC_TRIG_TYPE_EDGE;
    g_softInterruptHandle = HwiP_create(INTERRUPT_CTRL_SOFT_IRQ,
                                         handle_soft_int_prog,
                                         &params);
    if (g_softInterruptHandle == (HwiP_Handle)0)
    {
        return -1;
    }

    interrupt_ctrl_reload_timer();
    return 0;
}

void interrupt_ctrl_enable(void)
{
    __asm__ volatile("msr daifclr, #2" ::: "memory");
    __asm__ volatile("isb");
}
