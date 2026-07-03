/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       ddr_ecc.c
 *@author     LiuRui
 *@date       2026.05.18
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.05.18  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include <stdint.h>
#include <ti/csl/csl_types.h>
#include <ti/drv/uart/UART_stdio.h>
#include "ecc_ddr.h"
#include "ecc_msmc_mem_parity.h"
#include "ti/osal/src/printf.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

int32_t gDdrEccTestResult = CSL_EFAIL;
int32_t gMsmcMemParityResult = CSL_EFAIL;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/* The main code here initializes the platform and processes
 * commands through UART port.
 * The menu commands are hierarchical and starts with the main menu with
 * the class of tests. The test classes include OCMC ecc test, EMIF ecc test
 * and DSP ECC test.
 * The OCMC err test & EMIF err test functions are in thier respective
 * separate c files and are used by all cores.
 * Once the class of tests are selected, then the submenu options for
 * the chosen class, are processed.
 */
int ecc_test(void)
{
    gMsmcMemParityResult = msmcEccMemParityTest();
    if (gMsmcMemParityResult == CSL_PASS)
    {
        printf_("\r\nMSMC Memory Parity tests have passed.");
    }
    /* @description:Test runner for ECC AGGR tests
        @requirements: PDK-2433 PDK-5879
        @cores: mcu1_0 */
    gDdrEccTestResult = DDREccTest();
    if (gDdrEccTestResult != CSL_PASS)
    {
        printf_("\r\n DDR ECC test failed \n");
    }
}
