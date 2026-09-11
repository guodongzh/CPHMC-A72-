/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       main.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 No-OS GPIO output and shared external-interrupt test.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include <stdint.h>

#include "app/app_main.h"

void a72_main(void)
{
    if (app_main_init() != 0)
    {
        for (;;)
        {
            __asm__ volatile("wfe");
        }
    }
    app_main_run();
}
