/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       sd_power.c
 *@author     LiuRui
 *@date       2024.09.06
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.09.06  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include <ti/drv/gpio/GPIO.h>
#include <ti/csl/soc/j721e/src/cslr_soc.h>
#include <ti/csl/src/ip/gpio/V0/gpio.h>
#include "ti/drv/gpio/src/v0/GPIO_v0.h"
#include "ti/drv/gpio/soc/GPIO_soc.h"
#include "my_board_init.h"
#include <ti/csl/csl_intr_router.h>

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
/* GPIO Driver board specific pin configuration structure */
GPIO_PinConfig gpioPinConfigs[] = {
    /* Output pin */
    GPIO_DEVICE_CONFIG(GPIO_SD_PORT_NUM, GPIO_SD_PIN_NUM) |
        GPIO_CFG_OUTPUT,
};

/* GPIO Driver call back functions */
GPIO_CallbackFxn gpioCallbackFunctions[] = {
    NULL,
};

/* GPIO Driver configuration structure */
GPIO_v0_Config GPIO_v0_config = {
    .pinConfigs         = gpioPinConfigs,
    .callbacks          = gpioCallbackFunctions,
    .numberOfPinConfigs = sizeof(gpioPinConfigs) / sizeof(GPIO_PinConfig),
    .numberOfCallbacks  = sizeof(gpioCallbackFunctions) / sizeof(GPIO_CallbackFxn),
#if (__ARM_ARCH == 7) && (__ARM_ARCH_PROFILE == 'R') /* R5F */
    .intPriority = 0x8U
#else
#if defined(BUILD_C7X)
    0x01U
#else
    0x20U
#endif
#endif
};

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

int Board_pmPowerOff(uint32_t slaveAddr)
{
    return 0;
}

/**
 * @brief enable sd power
 */
void enable_sd_power(void)
{
    /* Call board init functions */
    GPIO_v0_HwAttrs gpio_cfg;

    /* Get the default GPIO init configurations */
    GPIO_socGetInitCfg(GPIO_SD_PORT_NUM, &gpio_cfg);

    /* change default GPIO port from MAIN GPIO0 to WAKEUP GPIO0 to access TP45 */
    gpio_cfg.baseAddr = CSL_GPIO0_BASE;

    GPIO_socSetInitCfg(GPIO_SD_PORT_NUM, &gpio_cfg);

    /* GPIO module initialization */
    GPIO_init();

    GPIO_write(GPIO_SD_PORT_NUM, GPIO_SD_PIN_NUM, GPIO_PIN_HIGH);
}

uint32_t mmr_unlock_one(uint32_t *kick0, uint32_t *kick1)
{
    /* initialize the status variable */
    uint32_t status = 1;

    /* if either of the kick lock registers are locked */
    if (!(*kick0 & 0x1) | !(*kick1 & 0x1))
    {
        /* unlock the partition by writing the unlock values to the kick lock registers */
        *kick0 = KICK0_UNLOCK_VAL;
        *kick1 = KICK1_UNLOCK_VAL;
    }

    /* check to see if either of the kick registers are unlocked. */
    if (!(*kick0 & 0x1))
    {
        status = 0;
    }

    /* return the status to the calling program */
    return status;
}

uint32_t main_mmr_unlock_all()
{
    /* initialize the status variable */
    uint32_t status = 1;
    /* Unlock the 0th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK0_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK0_KICK1));
    /* Unlock the 1st partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK1_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK1_KICK1));
    /* Unlock the 2nd partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK2_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK2_KICK1));
    /* Unlock the 3rd partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK3_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK3_KICK1));
    /* Unlock the 4th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK4_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK4_KICK1));
    /* Unlock the 5th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK5_KICK1));
    /* Unlock the 6th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK6_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK6_KICK1));

    /* Unlock the 7th partition */
    status &= mmr_unlock_one((uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK7_KICK0),
                             (uint32_t *)(MAIN_MMR_BASE_ADDRESS + CSL_MAIN_CTRL_MMR_CFG0_LOCK7_KICK1));
    /* Return status to calling program */
    return status;
}

void pcie_refclk_to_io(uint32_t ints_num, uint32_t ref_clk)
{
    switch (ints_num)
    {
    case 0:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK0_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK0_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE0_CTRL) &= 0xFFFFFFFC;
        break;
    case 1:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK1_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK1_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE0_CTRL) &= 0xFFFFFFFC;
        break;
    case 2:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK2_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK2_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE1_CTRL) &= 0xFFFFFFFC;
        break;
    case 3:
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK3_CLKSEL) = ref_clk;
        /* Enable output clock */
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE_REFCLK3_CLKSEL) |= 0x100;
        *(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + MAIN_CTRL_ACSPCIE1_CTRL) &= 0xFFFFFFFC;
        break;
    default:
        break;
    }
}

void pcie_set_mode(uint32_t ints_num, uint32_t rate,
                   uint32_t mode, uint32_t lane_count)
{
    switch (ints_num)
    {
    case 0:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE0_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    case 1:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE1_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE1_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE1_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    case 2:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE2_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE2_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE2_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    case 3:
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE3_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_GENERATION_SEL, 2);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE3_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_MODE_SEL, 1);
        CSL_FINS(*(uint32_t *)(CSL_CTRL_MMR0_CFG0_BASE + CSL_MAIN_CTRL_MMR_CFG0_PCIE3_CTRL),
                 MAIN_CTRL_MMR_CFG0_PCIE0_CTRL_LANE_COUNT, 1);
        break;
    default:
        break;
    }
}



void gpio_intr_init()
{
    volatile uint32_t *addr = (uint32_t *)(CSL_GPIOMUX_INTRTR0_MUXCNTL_START + (4 * 25));
    *addr = CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE | 23;

    addr = (uint32_t *)(CSL_GPIOMUX_INTRTR0_MUXCNTL_START + (4 * 29));
    *addr = CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE | 24;

    addr = (uint32_t *)(CSL_GPIOMUX_INTRTR0_MUXCNTL_START + (4 * 17));
    *addr = CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE | 25;

    addr = (uint32_t *)(CSL_GPIOMUX_INTRTR0_MUXCNTL_START + (4 * 21));
    *addr = CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE | 26;

    addr = (uint32_t *)(CSL_GPIOMUX_INTRTR0_MUXCNTL_START + (4 * 33));
    *addr = CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE | 27;

    addr = (uint32_t *)(CSL_GPIOMUX_INTRTR0_MUXCNTL_START + (4 * 37));
    *addr = CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE | 28;

    addr = (uint32_t *)(CSL_GPIOMUX_INTRTR0_MUXCNTL_START + (4 * 41));
    *addr = CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE | 29;

    addr = (uint32_t *)(CSL_C66SS0_INTRTR0_MUXCNTL_START + (4 * 61));
    *addr = CSL_C66SS0_INTRTR0_MUXCNTL_ENABLE | 392;

    addr = (uint32_t *)(CSL_C66SS1_INTRTR0_MUXCNTL_START + (4 * 61));
    *addr = CSL_C66SS1_INTRTR0_MUXCNTL_ENABLE | 396;
}

void intr_router_cfg()
{
    CSL_IntrRouterCfg irRegs;
    irRegs.pIntrRouterRegs = (CSL_intr_router_cfgRegs *)CSL_GPIOMUX_INTRTR0_INTR_ROUTER_CFG_BASE;
    irRegs.pIntdRegs = (CSL_intr_router_intd_cfgRegs *)NULL;
    irRegs.numInputIntrs = 303;
    irRegs.numOutputIntrs = 64;
    CSL_intrRouterCfgMux(&irRegs, CSLR_GPIOMUX_INTRTR0_IN_MAIN_GPIO0_VIRT_OUT0_23_0, 25);
    CSL_intrRouterCfgMux(&irRegs, CSLR_GPIOMUX_INTRTR0_IN_MAIN_GPIO0_VIRT_OUT0_24_0, 29);
    CSL_intrRouterCfgMux(&irRegs, CSLR_GPIOMUX_INTRTR0_IN_MAIN_GPIO0_VIRT_OUT0_25_0, 17);
    CSL_intrRouterCfgMux(&irRegs, CSLR_GPIOMUX_INTRTR0_IN_MAIN_GPIO0_VIRT_OUT0_26_0, 21);
    CSL_intrRouterCfgMux(&irRegs, CSLR_GPIOMUX_INTRTR0_IN_MAIN_GPIO0_VIRT_OUT0_27_0, 33);
    CSL_intrRouterCfgMux(&irRegs, CSLR_GPIOMUX_INTRTR0_IN_MAIN_GPIO0_VIRT_OUT0_28_0, 37);
    CSL_intrRouterCfgMux(&irRegs, CSLR_GPIOMUX_INTRTR0_IN_MAIN_GPIO0_VIRT_OUT0_29_0, 41);

    irRegs.pIntrRouterRegs = (CSL_intr_router_cfgRegs *)CSL_C66SS0_INTROUTER0_INTR_ROUTER_CFG_BASE;
    irRegs.pIntdRegs = (CSL_intr_router_intd_cfgRegs *)NULL;
    irRegs.numInputIntrs = 400;
    irRegs.numOutputIntrs = 64;
    CSL_intrRouterCfgMux(&irRegs, CSLR_C66SS1_INTROUTER0_IN_GPIOMUX_INTRTR0_OUTP_33, 61);

    irRegs.pIntrRouterRegs = (CSL_intr_router_cfgRegs *)CSL_C66SS1_INTROUTER0_INTR_ROUTER_CFG_BASE;
    irRegs.pIntdRegs = (CSL_intr_router_intd_cfgRegs *)NULL;
    irRegs.numInputIntrs = 400;
    irRegs.numOutputIntrs = 64;
    CSL_intrRouterCfgMux(&irRegs, CSLR_C66SS1_INTROUTER0_IN_GPIOMUX_INTRTR0_OUTP_37, 61);
}
