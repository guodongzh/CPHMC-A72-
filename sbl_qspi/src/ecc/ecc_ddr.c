#include <stdint.h>
#include <ti/osal/osal.h>
#include "ecc_ddr.h"
#include "board/board_cfg.h"

/* ----------------- Constant definitions ----------------- */

/* -------------------------------------------------------- */

/* ----------------- Function prototypes ------------------ */
static int32_t DDRSecErrTest();
static int32_t DDRDedErrTest();

/* -------------------------------------------------------- */

/* Function executes Emif ECC Test */
int32_t DDREccTest(void)
{
    int32_t sec_revt = CSL_EFAIL, ded_revt = CSL_EFAIL;
    sec_revt = DDRSecErrTest();
    ded_revt = DDRDedErrTest();

    return ((sec_revt != CSL_PASS) || (ded_revt != CSL_PASS));
}

uintptr_t DDRGetTranslatedAddress(uintptr_t memAddress)
{
    uint32_t memIndex;
    uintptr_t translatedMemAddr;

    memIndex = (memAddress - 0x80000000u) / EMIF_ECC_MEM_BLOCK_SIZE;
    if ((memIndex & 0x1u) == 0)
    {
        translatedMemAddr = memAddress + ((memIndex)*EMIF_ECC_DATA_SIZE_PER_BLOCK);
    }
    else
    {
        translatedMemAddr = memAddress + ((memIndex + 1u) * EMIF_ECC_DATA_SIZE_PER_BLOCK);
    }
    return translatedMemAddr;
}

/* Function performs DDR single bit error test
 * Configures ECC, Inserts single bit error
 * and waits for handler to finish
 */
static int32_t DDRSecErrTest()
{
    // single bit error
    volatile uint32_t *gTest_Addr = NULL;
    volatile uint32_t testVal;
    volatile uint32_t testVal2;
    volatile uint32_t *translatedMemPtr;
    uint32_t waitCount = 0;
    CSL_emif_sscfgRegs *pEmifSsRegs = (CSL_emif_sscfgRegs *)CSL_COMPUTE_CLUSTER0_SS_CFG_BASE;

    /* Clear any residual ECC errors */
    Board_STATUS status = BOARD_SOK;
    status = CSL_emifClearAllECCErrors(pEmifSsRegs);
    if (status == BOARD_SOK)
    {
        status =
            CSL_emifClearECCInterruptStatus(pEmifSsRegs,
                                            CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECC1BERR_EN_MASK |
                                                CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECCM1BERR_EN_MASK |
                                                CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECC2BERR_EN_MASK);
    }

    /* Inject error */
    gTest_Addr = (uint32_t *)DDR_ECC_TEST_ADDR;

    /* Write back any pending writes */
    CacheP_wbInv((void *)gTest_Addr, 4);

    /* Read value from test location */
    testVal = gTest_Addr[0];

    /* Flip one bit to introduce error */
    testVal2 = testVal ^ 0x00010000u;

    /* Calculate translated address */
    translatedMemPtr = (volatile uint32_t *)(DDRGetTranslatedAddress((uintptr_t)gTest_Addr));

    /* Generating a 1b ECC error */
    /* NOTE: The following section should NOT be useed in actual application */
    /* ================================================================================ */
#ifdef BUILD_MCU1_0
    /* Temporarily disable ECC */
    CSL_emifDisableECC(pEmifSsRegs);

    /* Now corrupt the value */
    *(translatedMemPtr) = testVal2;
    CacheP_wbInv((void *)translatedMemPtr, 4);

    /* Enable back ECC */
    CSL_emifEnableECC(pEmifSsRegs);
#endif
    /* ================================================================================ */

    /* Invalidate cache */
    CacheP_Inv((void *)gTest_Addr, 4);

    /* Read value to trigger error */
    testVal2 = gTest_Addr[0];

    /* wait until the test passes */
    printf_("\r\n Waiting on SEC ERR ...");
    while ((waitCount++ < 100u))
    {
        if (CSL_FEXT(pEmifSsRegs->V2A_INT_RAW_REG, EMIF_SSCFG_V2A_INT_RAW_REG_ECC1BERR))
        {
            break;
        }
        Osal_delay(10);
    }

    if (waitCount < 100u)
    {
        printf_("\r\n Got it (test pass)...");
        printf_("\r\nDDR ECC SEC ECC TESTS PASSED");
        return CSL_PASS;
    }
    else
    {
        printf_("\r\n Test failed timedout ...");
        printf_("\r\nDDR ECC SEC ECC TESTS FAILED");
        return CSL_EFAIL;
    }
}

/* Function performs DDR double bit error test
 * Inserts double bit error and waits for handler to finish
 */
static int32_t DDRDedErrTest()
{
    volatile uint32_t *gTest_Addr = NULL;
    volatile uint32_t testVal;
    volatile uint32_t testVal2;
    volatile uint32_t *translatedMemPtr;
    uint32_t waitCount = 0;
    CSL_emif_sscfgRegs *pEmifSsRegs = (CSL_emif_sscfgRegs *)CSL_COMPUTE_CLUSTER0_SS_CFG_BASE;

    // double bit error
    /* Clear any residual ECC errors */
    Board_STATUS status = BOARD_SOK;
    status = BOARD_SOK;
    status = CSL_emifClearAllECCErrors(pEmifSsRegs);
    if (status == BOARD_SOK)
    {
        status =
            CSL_emifClearECCInterruptStatus(pEmifSsRegs,
                                            CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECC1BERR_EN_MASK |
                                                CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECCM1BERR_EN_MASK |
                                                CSL_EMIF_SSCFG_V2A_INT_SET_REG_ECC2BERR_EN_MASK);
    }

    gTest_Addr = (uint32_t *)DDR_ECC_TEST_ADDR;

    CacheP_wbInv((void *)gTest_Addr, 4);
    /* Read reference value */
    testVal = gTest_Addr[0];
    /* flip 2 bits */
    testVal2 = testVal ^ 0x00101000u;
    /* Calculate translated address */
    translatedMemPtr = (volatile uint32_t *)(DDRGetTranslatedAddress((uintptr_t)gTest_Addr));

    /* Generating a 2b ECC error */
    /* NOTE: The following section should NOT be useed in actual application */
    /* ================================================================================ */
#ifdef BUILD_MCU1_0
    /* Temporarily disable ECC */
    CSL_emifDisableECC(pEmifSsRegs);

    /* Now corrupt the value */
    *(translatedMemPtr) = testVal2;

    /* Make sure the values are written back */
    CacheP_wbInv((void *)translatedMemPtr, 4);

    /* Enable back ECC */
    CSL_emifEnableECC(pEmifSsRegs);
#endif
    /* ================================================================================ */

    /* Invalidate cache */
    CacheP_Inv((void *)gTest_Addr, 4);

    /* Read value to trigger error */
    testVal2 = gTest_Addr[0];

    /* wait until the test passes
     * the cpu will be in data abort exception
     * */
    printf_("\r\n Waiting on DED ERR ...");
    waitCount = 0;
    while ((waitCount++ < 100u))
    {
        if (CSL_FEXT(pEmifSsRegs->V2A_INT_RAW_REG, EMIF_SSCFG_V2A_INT_RAW_REG_ECC2BERR))
        {
            break;
        }
        Osal_delay(10);
    }

    if (waitCount < 100u)
    {
        printf_("\r\n Got it (test pass)...");
        printf_("\r\nDDR ECC DED ECC TESTS PASSED");
        return CSL_PASS;
    }
    else
    {
        printf_("\r\n Test failed timedout ...");
        printf_("\r\nDDR ECC DED ECC TESTS FAILED");
        return CSL_EFAIL;
    }
}

/* Nothing past this point */
