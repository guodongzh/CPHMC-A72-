/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_intr_init.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      Shared GPIO external-interrupt implementation for A72 No-OS.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include "bsp/gpio/gpio_intr_init.h"

#include <ti/drv/gpio/GPIO.h>

#include "bsp/gpio/gpio_ctrl.h"

static volatile uint32_t g_gpioIntrCount;
static int32_t g_gpioIntrInitialized;

void gpio_intr_callback(void)
{
    g_gpioIntrCount++;
}

int32_t gpio_intr_init(void)
{
    if (g_gpioIntrInitialized != 0)
    {
        return 0;
    }

    g_gpioIntrCount = 0U;
    GPIO_clearInt(GPIO_CTRL_SHARED_INTR_INDEX);
    GPIO_enableInt(GPIO_CTRL_SHARED_INTR_INDEX);
    g_gpioIntrInitialized = 1;

    return 0;
}

uint32_t gpio_intr_get_count(void)
{
    return g_gpioIntrCount;
}
