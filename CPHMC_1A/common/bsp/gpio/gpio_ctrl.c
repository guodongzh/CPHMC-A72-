/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_ctrl.c
 *@author     LiuRui
 *@date       2025.11.17
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.11.17  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "gpio_ctrl.h"
#include "ti/csl/soc/j721e/src/cslr_soc.h"
#include <ti/csl/src/intc/csl_intc.h>
#include <ti/csl/csl_clec.h>

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

#if !defined(BUILD_C66X)
/* GPIO Driver board specific pin configuration structure */
GPIO_PinConfig gpioPinConfigs[] = {
#if defined(BUILD_MCU2_0)

    /* Input pin with interrupt enabled */
    /* io interrupt pin num*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT) | GPIO_CFG_IN_INT_FALLING | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT_C66_0) | GPIO_CFG_IN_INT_FALLING | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT_C66_1) | GPIO_CFG_IN_INT_FALLING | GPIO_CFG_INPUT,

    /*spi cfg*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SPI_FPGA0) | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SPI_FPGA1) | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,

/*88e1512 phy enable output pin*/
//    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTA_EN) | GPIO_CFG_OUTPUT,
//    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTB_EN) | GPIO_CFG_OUTPUT,
//    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTC_EN) | GPIO_CFG_OUTPUT,
//    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTD_EN) | GPIO_CFG_OUTPUT,

/*ssd ctrl*/
//    GPIO_DEVICE_CONFIG(0, PIN_NUM_PCIE_SUSCLK) |
//        GPIO_CFG_OUTPUT | GPIO_CFG_OUT_HIGH,
//    GPIO_DEVICE_CONFIG(0, PIN_NUM_PCIE_PERST) |
//        GPIO_CFG_OUTPUT | GPIO_CFG_OUT_HIGH,

    /*flash ctrl output pin*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_FLASH_CTRL1) | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_FLASH_CTRL0) | GPIO_CFG_OUTPUT,

#elif defined(BUILD_MCU2_1)

    /* Input pin with interrupt enabled */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT) | GPIO_CFG_IN_INT_FALLING | GPIO_CFG_INPUT,

#elif defined(BUILD_MCU3_0)

    /* Input pin with interrupt enabled */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT) | GPIO_CFG_IN_INT_FALLING | GPIO_CFG_INPUT,

    /* uart txen output pin*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_UART8_TXEN) | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_UART9_TXEN) | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,

    /*relay clk output pin*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SOC_DO_CHG) | GPIO_CFG_OUTPUT,

    /*relay ctrl output pin*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SOC_ALM_CTL0) | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SOC_ALM_CTL1) | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SOC_FAIL_CTL0) | GPIO_CFG_OUTPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SOC_FAIL_CTL1) | GPIO_CFG_OUTPUT,

#elif defined(BUILD_MCU3_1)

    /* Input pin with interrupt enabled */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT) | GPIO_CFG_IN_INT_FALLING | GPIO_CFG_INPUT,

#elif defined(BUILD_C7X_1)
    /* Input pin with interrupt enabled */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT) | GPIO_CFG_IN_INT_RISING | GPIO_CFG_INPUT,

#elif defined(BUILD_MCU1_1)
    /* Input pin with interrupt enabled */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_INT) | GPIO_CFG_IN_INT_RISING | GPIO_CFG_INPUT,
#endif

    /*fpga wdt */
    GPIO_DEVICE_CONFIG(1, PIN_NUM_WDT0) | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,
    GPIO_DEVICE_CONFIG(1, PIN_NUM_WDT1) | GPIO_CFG_OUTPUT | GPIO_CFG_OUT_LOW,

    /*front panel buttons*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PBTN0) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PBTN1) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PBTN2) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PBTN3) | GPIO_CFG_INPUT,

    /* front panel Input pin */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PEXIST) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PFUNC0) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PFUNC1) | GPIO_CFG_INPUT,

    /* PWRA */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRA_EX) | GPIO_CFG_IN_PU,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRA_FALA) | GPIO_CFG_IN_PD,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRA_FALB) | GPIO_CFG_IN_PD,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRA_TMP) | GPIO_CFG_IN_PD,

    /* PWRB */
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRB_EX) | GPIO_CFG_IN_PU,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRB_FALA) | GPIO_CFG_IN_PD,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRB_FALB) | GPIO_CFG_IN_PD,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_PWRB_TMP) | GPIO_CFG_IN_PD,

    /* slot online Input pin*/
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTA_EX) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTB_EX) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTC_EX) | GPIO_CFG_INPUT,
    GPIO_DEVICE_CONFIG(0, PIN_NUM_SLOTD_EX) | GPIO_CFG_INPUT,
};

/* GPIO Driver call back functions */

GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,
};

/* GPIO Driver configuration structure */
GPIO_v0_Config GPIO_v0_config = {
    .pinConfigs = gpioPinConfigs,
    .callbacks = gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs) / sizeof(GPIO_PinConfig),
    .numberOfCallbacks = sizeof(gpioCallbackFunctions) / sizeof(GPIO_CallbackFxn),
#if (__ARM_ARCH == 7) && (__ARM_ARCH_PROFILE == 'R') /* R5F */
    .intPriority = 0x8U
#else
#if defined(BUILD_C7X)
    .intPriority = 0x01U
#else
    .intPriority = 0x20U
#endif
#endif
};
#endif

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
#if !defined(BUILD_C66X)
void gpio_ctrl_init(void)
{
    GPIO_v0_HwAttrs gpio_cfg;

    /* set main domain gpio cfg */
    GPIO_socGetInitCfg(0, &gpio_cfg);
    gpio_cfg.baseAddr = CSL_GPIO0_BASE;
    GPIO_socSetInitCfg(0, &gpio_cfg);

    /* set wkup domain gpio cfg */
    GPIO_socGetInitCfg(1, &gpio_cfg);
    gpio_cfg.baseAddr = CSL_WKUP_GPIO0_BASE;
    GPIO_socSetInitCfg(1, &gpio_cfg);

    /* GPIO module initialization */
    GPIO_init();
}
#endif

void gpio_get_buttons_status(void)
{
    uint32_t val = 0;
    val = GPIO_read(0, PIN_NUM_PBTN0);
    Debug_logTag("PBTN0 : %d\r\n", val);
    val = GPIO_read(0, PIN_NUM_PBTN1);
    Debug_logTag("PBTN1 : %d\r\n", val);
    val = GPIO_read(0, PIN_NUM_PBTN2);
    Debug_logTag("PBTN2 : %d\r\n", val);
    val = GPIO_read(0, PIN_NUM_PBTN3);
    Debug_logTag("PBTN3 : %d\r\n", val);
    Debug_log("\r\n");
}

void gpio_enable_rs485_uart8_txen(bool enable)
{
    if (enable)
    {
        GPIO_write(0, PIN_NUM_UART8_TXEN, GPIO_PIN_HIGH);
    }
    else
    {
        GPIO_write(0, PIN_NUM_UART8_TXEN, GPIO_PIN_LOW);
    }
}

void gpio_enable_rs485_uart9_txen(bool enable)
{
    if (enable)
    {
        GPIO_write(0, PIN_NUM_UART9_TXEN, GPIO_PIN_HIGH);
    }
    else
    {
        GPIO_write(0, PIN_NUM_UART9_TXEN, GPIO_PIN_LOW);
    }
}

void gpio_do0_on(void)
{
    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_LOW);

    GPIO_write(0, PIN_NUM_SOC_ALM_CTL0, GPIO_PIN_HIGH);
    GPIO_write(0, PIN_NUM_SOC_ALM_CTL1, GPIO_PIN_HIGH);

    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_HIGH);
}

void gpio_do1_on(void)
{
    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_LOW);

    GPIO_write(0, PIN_NUM_SOC_FAIL_CTL0, GPIO_PIN_LOW);
    GPIO_write(0, PIN_NUM_SOC_FAIL_CTL1, GPIO_PIN_HIGH);

    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_HIGH);
}

void gpio_do0_off(void)
{
    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_LOW);

    GPIO_write(0, PIN_NUM_SOC_ALM_CTL0, GPIO_PIN_LOW);
    GPIO_write(0, PIN_NUM_SOC_ALM_CTL1, GPIO_PIN_HIGH);

    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_HIGH);
}

void gpio_do1_off(void)
{
    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_LOW);

    GPIO_write(0, PIN_NUM_SOC_FAIL_CTL0, GPIO_PIN_HIGH);
    GPIO_write(0, PIN_NUM_SOC_FAIL_CTL1, GPIO_PIN_HIGH);

    GPIO_write(0, PIN_NUM_SOC_DO_CHG, GPIO_PIN_HIGH);
}


void gpio_wdt_toggle(void)
{
    GPIO_toggle(1, PIN_NUM_WDT0);
    GPIO_toggle(1, PIN_NUM_WDT1);
}


