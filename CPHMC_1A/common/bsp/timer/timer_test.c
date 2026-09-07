/**
 *************************************************************************
 * @file      timer.c
 * @author    LiuRui
 * @date      2024/5/24
 * @version   V1.0
 * @board     ti_j721e_evm
 * @brief     harderwear timer test
 *************************************************************************
 */

#include "timer_test.h"


TimerP_Handle timerHandle;

void Timer_IsrNULL(uintptr_t args)
{

}

/**
 * @brief harderwear timer test
 */
void timer_test()
{
    TimerP_Params timerParams;

    /* create harderwear timer */
    TimerP_Params_init(&timerParams);
    timerParams.runMode = TimerP_RunMode_CONTINUOUS;
    timerParams.startMode = TimerP_StartMode_AUTO;
    timerParams.periodType = TimerP_PeriodType_MICROSECS;
    timerParams.period = 2000; //2ms
    timerHandle = TimerP_create(TimerP_ANY, Timer_IsrNULL, &timerParams);
}
