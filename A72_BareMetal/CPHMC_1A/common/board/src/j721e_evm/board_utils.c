/******************************************************************************
 * Copyright (c) 2019-2023 Texas Instruments Incorporated - http://www.ti.com
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *****************************************************************************/

/**
 *  \file   board_utils.c
 *
 *  \brief  Implements multiple board utility functions
 *
 */

#include <stdio.h>
#include <string.h>
#include <ti/csl/arch/csl_arch.h>
#include "board/src/j721e_evm/include/board_internal.h"
#include "board/src/j721e_evm/include/board_utils.h"
#include "board/src/j721e_evm/include/board_info_ddr.h"
#include "board/src/j721e_evm/include/board_cfg.h"

Board_DetectCfg_t  gBoardDetCfg[BOARD_ID_MAX_BOARDS] =
 {{BOARD_COMMON_EEPROM_I2C_INST, BOARD_GESI_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_WKUP, "J7X-GESI-EXP"},
  {BOARD_COMMON_EEPROM_I2C_INST, BOARD_GESI_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_WKUP, "J7X-INFOTAN-EXP"},
  {BOARD_CSI2_EEPROM_I2C_INST, BOARD_CSI2_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_MAIN, "J7X-FUSION2-CSI"},
  {BOARD_CSI2_EEPROM_I2C_INST, BOARD_CSI2_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_MAIN, "J7X-LIIPEX-CSI"},
  {BOARD_CSI2_EEPROM_I2C_INST, BOARD_CSI2_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_MAIN, "J7X-LIMIPI-CSI"},
  {BOARD_COMMON_EEPROM_I2C_INST, BOARD_ENET_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_WKUP, "J7X-VSC8514-ETH"},
  {BOARD_COMMON_EEPROM_I2C_INST, BOARD_DISPLAY_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_WKUP, "TBD"},
  {BOARD_COMMON_EEPROM_I2C_INST, BOARD_SOM_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_WKUP, "J721EX-PM2-SOM"},
  {BOARD_COMMON_EEPROM_I2C_INST, BOARD_CP_EEPROM_SLAVE_ADDR, BOARD_SOC_DOMAIN_WKUP, "J7X-BASE-CPB"}};

Board_I2cInitCfg_t gBoardI2cInitCfg = {0U, BOARD_SOC_DOMAIN_MAIN, BFALSE};
Board_initParams_t gBoardInitParams = {BOARD_UART_INSTANCE, BOARD_UART_SOC_DOMAIN, BOARD_PSC_DEVICE_MODE_NONEXCLUSIVE,
                                       BOARD_MAIN_CLOCK_GROUP_ALL, BOARD_MCU_CLOCK_GROUP_ALL};

/* Variables to store and restore the RAT configurations on DSP core */
#if defined (_TMS320C6X)
static uint32_t gRatOffsetHi;
static uint32_t gRatOffsetLo;
static uint32_t gRatCfg;
#endif

/**
 * \brief Function to configure I2C configurations used by board
 *
 * This function is used to set the I2C controller instance and
 * SoC domain used by the board module for IO expander and board
 * ID info read.
 *
 * Usage:
 * Call Board_setI2cInitConfig to set the I2C configurations
 * Call IO expander Init or Board ID info read/write
 *
 *  \return   BOARD_SOK in case of success or appropriate error code.
 *
 */
Board_STATUS Board_setI2cInitConfig(Board_I2cInitCfg_t *i2cCfg)
{
    if(NULL == i2cCfg)
    {
        return BOARD_INVALID_PARAM;
    }

    gBoardI2cInitCfg = *i2cCfg;

    return BOARD_SOK;
}


/**
 * \brief Configures drive strength for LVCMOS IOs
 *
 * This is temporary function added as workaround for
 * eFuse drive strength configuration issue. Need to be
 * removed after this issue is fixed in the silicon.
 *
 */
void Board_lvcmosDrvStrengthCfg(void)
{
    /* Unlock MMR */
    Board_unlockMMR();

    /* Configure Horizontal_Drive_Strength 3 to 0 */
    /* Configure only if the IO strength is not as expected */
    if((HW_RD_REG32(CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40C0)) < 0xD)
    {
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40C0), 0xD);
        /* Configure Horizontal_Drive_Strength 7 to 4 */
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40C4), 0xD);
        /* Configure Horizontal_Drive_Strength 11 to 8 */
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40C8), 0xD);
        /* Configure Horizontal_Drive_Strength 15 to 12 */
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40CC), 0xD);
    }

    if((HW_RD_REG32(CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40D0)) < 0xD)
    {
        /* Configure Vertical_Drive_Strength 3 to 0 */
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40D0), 0xD);
        /* Configure Vertical_Drive_Strength 7 to 4 */
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40D4), 0xD);
        /* Configure Vertical_Drive_Strength 11 to 8 */
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40D8), 0xD);
        /* Configure Vertical_Drive_Strength 15 to 12 */
        HW_WR_REG32((CSL_WKUP_CTRL_MMR0_CFG0_BASE + 0x40DC), 0xD);
    }
}

/**
 * \brief Function to get board init params
 *
 *  This function shall be used to know the current board init
 *  parameters and update them if needed using the function Board_setInitParams.
 *
 * \param   initParams  [IN]  Board init params structure
 *
 * \return   BOARD_SOK in case of success or appropriate error code.
 *
 */
Board_STATUS Board_getInitParams(Board_initParams_t *initParams)
{
    if(NULL == initParams)
    {
        return BOARD_INVALID_PARAM;
    }

    *initParams = gBoardInitParams;

    return BOARD_SOK;
}

/**
 * \brief Function to configure board init parameters
 *
 *  Board init params includes the parameters used by Board_init
 *  function for different operations. Default init parameters
 *  used by Board_init can be updated using this function.
 *  All the default params can be overwritten by calling this function
 *  or some of can be changed by reading the existing init parameters
 *  using Board_getInitParams function.
 *
 * Usage:
 * Call Board_getInitParams to get the default board init parameters
 * Update the parameters as needed
 * Call Board_setInitParams to update the default board init parameters
 *
 * \param   initCfg  [IN]  Board Init config structure
 *
 * \return   BOARD_SOK in case of success or appropriate error code.
 *
 */
Board_STATUS Board_setInitParams(Board_initParams_t *initParams)
{
    if(NULL == initParams)
    {
        return BOARD_INVALID_PARAM;
    }

    gBoardInitParams = *initParams;

    return BOARD_SOK;
}

/**
 * \brief Function to get the SoC domain
 *
 *  This function returns the domain of the SoC core on which
 *  it is executing.
 *
 * \return   SoC domain of the core.
 *
 */
uint32_t Board_getSocDomain(void)
{
    uint32_t socDomain = BOARD_SOC_DOMAIN_MAIN;

#ifdef BUILD_MCU
    CSL_ArmR5CPUInfo info = {0};

    CSL_armR5GetCpuID(&info);
    if (CSL_ARM_R5_CLUSTER_GROUP_ID_0 == info.grpId)
    {
        socDomain = BOARD_SOC_DOMAIN_MCU;
    }
#endif

  return socDomain;
}

/**
 *  \brief  Sets RAT configuration
 *
 *  MAIN padconfig registers are not directly accessible for C66x core
 *  which requires RAT configuration for the access.
 *
 *  \return   None
 */
void Board_setRATCfg(void)
{
#if defined (_TMS320C6X)
    gRatOffsetLo = HW_RD_REG32(CSL_C66_COREPAC_C66_RATCFG_BASE + 0x24);
    gRatOffsetHi = HW_RD_REG32(CSL_C66_COREPAC_C66_RATCFG_BASE + 0x28);
    gRatCfg      = HW_RD_REG32(CSL_C66_COREPAC_C66_RATCFG_BASE + 0x20);
    HW_WR_REG32((CSL_C66_COREPAC_C66_RATCFG_BASE + 0x24),
                BOARD_C66X_RAT_OFFSET);
    HW_WR_REG32((CSL_C66_COREPAC_C66_RATCFG_BASE + 0x28), 0);
    HW_WR_REG32((CSL_C66_COREPAC_C66_RATCFG_BASE + 0x20),
                BOARD_C66X_RAT_CONFIG);
#endif
}

/**
 *  \brief  Restores RAT configuration
 *
 *  \return   None
 */
void Board_restoreRATCfg(void)
{
#if defined (_TMS320C6X)
    HW_WR_REG32((CSL_C66_COREPAC_C66_RATCFG_BASE + 0x20), gRatCfg);
    HW_WR_REG32((CSL_C66_COREPAC_C66_RATCFG_BASE + 0x24), gRatOffsetLo);
    HW_WR_REG32((CSL_C66_COREPAC_C66_RATCFG_BASE + 0x28), gRatOffsetHi);
#endif
}

/**
 *  \brief    Function to generate delay in micro seconds
 *
 *  This function takes the delay parameters in usecs but the generated
 *  delay will be in multiples of msecs due to the osal function which
 *  generates delay in msecs. Delay parameter passed will be converted to
 *  msecs and fractional value will be adjusted to nearest msecs value.
 *  Minimum delay generated by this function is 1 msec.
 *  Function parameter is kept in usecs to match with existing
 *  platforms which has delay function for usecs.
 *
 *  \param    usecs [IN]  Specifies the time to delay in micro seconds.
 *
 */
void BOARD_delay(uint32_t usecs)
{
    uint32_t msecs;

    msecs = usecs/1000;
    if(usecs%1000)
    {
        msecs += 1;
    }

    Osal_delay(msecs);
}
