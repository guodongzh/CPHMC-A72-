/******************************************************************************
 * Copyright (c) 2019-2020 Texas Instruments Incorporated - http://www.ti.com
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
 *  \file board_lld_init.c
 *
 *  \brief This file initializes UART and I2C LLD modules 
 *
 */

#include "board/src/j721e_evm/include/board_internal.h"
#include "board/src/j721e_evm/include/board_utils.h"
#include "board/src/j721e_evm/include/board_cfg.h"

extern Board_I2cInitCfg_t gBoardI2cInitCfg;
extern Board_initParams_t gBoardInitParams;
static uint32_t gUARTBaseAddr = 0U;
static uint32_t gUARTClkFreq = 0U;

uint32_t gBoardI2cBaseAddr[BOARD_SOC_DOMAIN_MAX][I2C_HWIP_MAX_CNT] =
    {{CSL_I2C0_CFG_BASE, CSL_I2C1_CFG_BASE, CSL_I2C2_CFG_BASE, CSL_I2C3_CFG_BASE,
      CSL_I2C4_CFG_BASE, CSL_I2C5_CFG_BASE, CSL_I2C6_CFG_BASE},
     {CSL_WKUP_I2C0_CFG_BASE, 0, 0, 0, 0, 0, 0},
     {CSL_MCU_I2C0_CFG_BASE, CSL_MCU_I2C1_CFG_BASE, 0, 0, 0, 0, 0}};

Board_I2cObj_t gBoardI2cObj[BOARD_I2C_PORT_CNT] = {
    {NULL, BOARD_SOC_DOMAIN_MAIN, 0, 0},
    {NULL, BOARD_SOC_DOMAIN_MAIN, 1, 0},
    {NULL, BOARD_SOC_DOMAIN_MAIN, 2, 0},
    {NULL, BOARD_SOC_DOMAIN_MAIN, 3, 0},
    {NULL, BOARD_SOC_DOMAIN_MAIN, 4, 0},
    {NULL, BOARD_SOC_DOMAIN_MAIN, 5, 0},
    {NULL, BOARD_SOC_DOMAIN_MAIN, 6, 0}
};

uint32_t gBoardUartBaseAddr[BOARD_SOC_DOMAIN_MAX][CSL_UART_PER_CNT] =
    {{CSL_UART0_BASE, CSL_UART1_BASE, CSL_UART2_BASE, CSL_UART3_BASE, CSL_UART4_BASE,
      CSL_UART5_BASE, CSL_UART6_BASE, CSL_UART7_BASE, CSL_UART8_BASE, CSL_UART9_BASE},
     {CSL_WKUP_UART0_BASE, 0, 0, 0, 0, 0, 0, 0, 0, 0},
     {CSL_MCU_UART0_BASE, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

/**
  *  \brief   Returns base address of given I2C instance
  *
  *  \param   instNum [IN] I2C instance
  *
  *  \param   domain  [IN] Domain of I2C controller
  *                           BOARD_SOC_DOMAIN_MAIN - Main Domain
  *                           BOARD_SOC_DOMAIN_WKUP - Wakeup domain
  *                           BOARD_SOC_DOMAIN_MCU - MCU domain
  * 
  *  \return  Valid base address in case of success or 0
  *
  */
static uint32_t Board_getI2cBaseAddr(uint8_t instNum,
                                     uint8_t domain)
{
    uint32_t baseAddr = 0U;

    if((I2C_HWIP_MAX_CNT > instNum) &&
       (BOARD_SOC_DOMAIN_MCU >= domain))
    {
        baseAddr = gBoardI2cBaseAddr[domain][instNum];
    }

    return (baseAddr);
}

/**
  *  \brief   Returns base address of given UART instance
  *
  *  \param   instNum [IN] UART instance
  *
  *  \param   domain  [IN] Domain of UART controller
  *                           BOARD_SOC_DOMAIN_MAIN - Main Domain
  *                           BOARD_SOC_DOMAIN_WKUP - Wakeup domain
  *                           BOARD_SOC_DOMAIN_MCU - MCU domain
  * 
  *  \return  Valid base address in case of success or 0
  *
  */
static uint32_t Board_getUartBaseAddr(uint8_t instNum,
                                      uint8_t domain)
{
    uint32_t baseAddr = 0U;

    if((CSL_UART_PER_CNT > instNum) &&
       (BOARD_SOC_DOMAIN_MCU >= domain))
    {
        baseAddr = gBoardUartBaseAddr[domain][instNum];
    }

    return (baseAddr);
}

/**
 *  \brief   This function initializes the default UART instance for use for
 *           console operations.
 *
 *  \return  Board_STATUS in case of success or appropriate error code.
 *
 */
Board_STATUS Board_uartStdioInit(void)
{
    UART_HwAttrs uart_cfg;
    uint32_t uartInst;
    uint32_t uartBaseAddr;

    uint32_t socDomainCore;

    uartInst      = gBoardInitParams.uartInst;
    socDomainCore = Board_getSocDomain();
    if (socDomainCore == BOARD_SOC_DOMAIN_MCU)
    {
        socDomainCore = BOARD_SOC_DOMAIN_WKUP;
    }

    gBoardInitParams.uartSocDomain =  socDomainCore;

    /* Disable the UART interrupt */
    UART_socGetInitCfg(uartInst, &uart_cfg);

    uartBaseAddr = Board_getUartBaseAddr(uartInst, socDomainCore);
    if (0U != uartBaseAddr)
    {
        gUARTBaseAddr     = uart_cfg.baseAddr;
        uart_cfg.baseAddr = uartBaseAddr;
    }
    else
    {
        return BOARD_INVALID_PARAM;
    }
    if (socDomainCore == BOARD_SOC_DOMAIN_WKUP)
    {
        uart_cfg.frequency = BOARD_UART_CLK_WKUP;
    }
    else
    {
        uart_cfg.frequency = BOARD_UART_CLK_MAIN;
    }


    gUARTClkFreq = uart_cfg.frequency;
    uart_cfg.enableInterrupt = UFALSE;
    UART_socSetInitCfg(uartInst, &uart_cfg);

    UART_stdioInit(uartInst);

    return BOARD_SOK;
}

/**
 *  \brief   This function is used to de-initialize board UART handles.
 */
Board_STATUS Board_uartDeInit(void)
{
    UART_HwAttrs uart_cfg;
    uint32_t socDomainCore;
    
    UART_stdioDeInit();

    socDomainCore = Board_getSocDomain();

    if(gBoardInitParams.uartSocDomain != socDomainCore)
    {
        UART_socGetInitCfg(gBoardInitParams.uartInst, &uart_cfg);
        uart_cfg.baseAddr  = gUARTBaseAddr;
        uart_cfg.frequency = gUARTClkFreq;
        UART_socSetInitCfg(gBoardInitParams.uartInst, &uart_cfg);
    }

    return BOARD_SOK;
}
