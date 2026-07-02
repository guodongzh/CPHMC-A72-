/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       handle_int_prog.h
 *@author     LiuRui
 *@date       2025.10.21
 *@brief      Provide application layer interruption templates.
 *@par        History
 *Date        Version   Author     Description
 *2025.10.21  1.0       LiuRui     Initial version.
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "ti/osal/RegisterIntr.h"
#include "handle_int_prog.h"
#include "app_register.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */
static void soft_int_function(void);

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
// 修改应用层逻辑时不可修改此函数

void handle_hard_int_prog(void)
{


}

// 修改应用层逻辑时不可修改此函数
void handle_soft_int_prog(void)
{
    soft_int_function();

    for (uint8_t i = 0; i < task_cfg.loop_task_number; i++)
    {
        task_loop_rt[i].task_time++;
    }

    for (uint8_t i = 0; i < task_cfg.intr_task_number; i++)
    {
        task_intr_rt[i].task_time++;
    }

    // 以驱动层软中断的周期为时间片，触发应用层注册的中断
    for (uint8_t i = 0; i < task_cfg.intr_task_number; i++)
    {
        if ((task_intr_rt[i].task_time * SOFT_INT_TIME_US) >= task_cfg.intr_task_def[i].task_period_time_us)
        {
            HwiP_post(intr_num[i]);
            task_intr_rt[i].task_time = 0;
        }
    }
}

// 在软中断中需要处理的事情
static void soft_int_function(void)
{
}
