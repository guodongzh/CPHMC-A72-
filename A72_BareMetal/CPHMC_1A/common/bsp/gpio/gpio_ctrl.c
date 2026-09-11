/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_ctrl.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      GPIO control implementation for the A72 BareMetal.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#include "bsp/gpio/gpio_ctrl.h"


static GPIO_PinConfig g_gpioCtrlPinConfigs[] =
{
    GPIO_DEVICE_CONFIG(GPIO_CTRL_PORT, GPIO_CTRL_SHARED_INTR_PIN) |
        GPIO_CFG_INPUT | GPIO_CFG_IN_INT_RISING,
    GPIO_DEVICE_CONFIG(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE0_PIN) |
        GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,
    GPIO_DEVICE_CONFIG(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE2_PIN) |
        GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW
};

static GPIO_CallbackFxn g_gpioCtrlCallbacks[] =
{
    gpio_intr_callback,
    (GPIO_CallbackFxn)0,
    (GPIO_CallbackFxn)0
};

GPIO_v0_Config GPIO_v0_config =
{
    g_gpioCtrlPinConfigs,
    g_gpioCtrlCallbacks,
    sizeof(g_gpioCtrlPinConfigs) / sizeof(GPIO_PinConfig),
    sizeof(g_gpioCtrlCallbacks) / sizeof(GPIO_CallbackFxn),
    0x20U
};

static int32_t g_gpioCtrlInitialized;

int32_t gpio_ctrl_init(void)
{
    GPIO_v0_HwAttrs gpioCfg;

    if (g_gpioCtrlInitialized != 0)
    {
        return 0;
    }

    /* Use the TI GPIO LLD configuration for MAIN GPIO0. */
    if (GPIO_socGetInitCfg(GPIO_CTRL_PORT, &gpioCfg) != 0)
    {
        return -1;
    }
    gpioCfg.baseAddr = CSL_GPIO0_BASE;

    /*
     * TBL already routes GPIO0_29 to GPIOMUX_INTRTR0_OUTP_41 for DSPC7X.
     * Reuse that route on A72 instead of asking Sciclient to allocate a
     * second GPIO route, so both cores observe the same external event.
     */
    gpioCfg.intCfg[GPIO_CTRL_SHARED_INTR_PIN].intNum =
        GPIO_CTRL_SHARED_INTR_IRQ;
    gpioCfg.intCfg[GPIO_CTRL_SHARED_INTR_PIN].eventId = 0U;
    gpioCfg.intCfg[GPIO_CTRL_SHARED_INTR_PIN].intcMuxNum =
        INVALID_INTC_MUX_NUM;
    gpioCfg.socConfigIntrPath = (GPIO_socCfgIntrPathFxn)0;

    if (GPIO_socSetInitCfg(GPIO_CTRL_PORT, &gpioCfg) != 0)
    {
        return -1;
    }

    GPIO_init();
    g_gpioCtrlInitialized = 1;
    return 0;
}

