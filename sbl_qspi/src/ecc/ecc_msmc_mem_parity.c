#include <stdint.h>
#include <ti/osal/osal.h>
#include "ecc_msmc.h"
#include "ecc_msmc_mem_parity.h"

/* ----------------- Constant definitions ----------------- */

/* -------------------------------------------------------- */

/* ----------------- Local Function prototypes ------------------ */

/* -------------------------------------------------------- */

/* ----------------- Global variables ----------------- */

/* -----------Function executes memory parity ECC Test ----------- */
int32_t msmcEccMemParityTest(void)
{
    volatile Bool gMsmcMemParityInterrupt = FALSE;
    int32_t retVal = CSL_PASS;
    bool ratRetVal;
    CSL_ecc_aggrRegs *pEccAggrRegs;
    CSL_Ecc_AggrEDCInterconnectErrorInfo forceErr;
    uint32_t ramId = CC_MSMC_WRAP_ECC_AGGR0_MSMC_DATA_RAM_ID;
    CSL_Ecc_AggrEccRamErrorStatusInfo ramEccErrorStatus;
    CSL_Ecc_AggrEDCInterconnectErrorStatusInfo errStatusInfo;
    uint32_t maxTimeOutMilliSeconds = 3000;

    gMsmcMemParityInterrupt = FALSE;
    printf_("\r\n\r\n**** MSMC Memory Parity TEST ****");

#ifndef BUILD_C7X
    CSL_RatTranslationCfgInfo translationCfg;
    /* Add RAT configuration to access address > 32bit address range */
    translationCfg.translatedAddress = CSL_COMPUTE_CLUSTER0_MSMC_ECC_AGGR0_BASE;
    translationCfg.sizeInBytes = CSL_COMPUTE_CLUSTER0_MSMC_ECC_AGGR0_SIZE;
    translationCfg.baseAddress = (uint32_t)MSMC_PARITY_ECC_AGGR_REGION_LOCAL_BASE;

    /* Set up RAT translation */
    ratRetVal = CSL_ratConfigRegionTranslation((CSL_ratRegs *)MSMC_PARITY_ECC_AGGR_RAT_CFG_BASE,
                                               MSMC_PARITY_ECC_AGGR_RAT_REGION_INDEX,
                                               &translationCfg);
    if (ratRetVal == false)
    {
        return CSL_EFAIL;
    }
    else
    {
        pEccAggrRegs = (CSL_ecc_aggrRegs *)MSMC_PARITY_ECC_AGGR_REGION_LOCAL_BASE;
    }
#else
    pEccAggrRegs = (CSL_ecc_aggrRegs *)CSL_COMPUTE_CLUSTER0_MSMC_ECC_AGGR0_BASE;
#endif
    /*
        1.  Write the ecc vector register (aggregator address 0x8) with the
            index of the safety controller endpoint, make sure trigger_read=0.
        2.  Write to error1 register (addres 0x18) of the
            safety controller to program:
            2a.  the ecc_grp (group of checker to inject).
            2b.  The bit location that will be flipped. If the signal is N bits,
            then bit[N:N+p] is the parity bits. Bit location that can be
            injected with error is 0...N+p-1.
        3.  Write to the control register (address 0x14) of the
            safety controller to start the injection:
              3a.  Make sure ecc_check field is enabled
              3b.  Set force_se for single bit error
    */
    /* Configure ECC check */

    retVal = CSL_ecc_aggrConfigEDCInterconnect(pEccAggrRegs, ramId, TRUE);

    /* Verify configuration */
    if (retVal == CSL_PASS)
    {
        retVal = CSL_ecc_aggrVerifyConfigEDCInterconnect(pEccAggrRegs, ramId, TRUE);
    }
    if (retVal != CSL_PASS)
    {
        printf_("\r\nECC AGGR Configuration failed...");
    }

    bool error_check = true;
    retVal = CSL_ecc_aggrConfigEDCInterconnect(pEccAggrRegs, ramId, error_check);

    uint32_t pRegVal;
    CSL_ecc_aggrReadEccRamCtrlReg(pEccAggrRegs, ramId, &pRegVal);
    pRegVal |= 0x187;
    CSL_ecc_aggrWriteEccRamCtrlReg(pEccAggrRegs, ramId, pRegVal);

    pRegVal = 0;
    CSL_ecc_aggrReadEccRamCtrlReg(pEccAggrRegs, ramId, &pRegVal);

    if (retVal == CSL_PASS)
    {
        forceErr.intrSrc = CSL_ECC_AGGR_INTR_SRC_SINGLE_BIT;
        forceErr.eccGroup = MSMC_ECC_PARITY_ECC_GROUP_ACCESS_SEL; /* This group covers the Parity
                                                                     Check for 16 Bits */
        forceErr.eccBit1 = MSMC_ECC_ERR_BIT_1;
        forceErr.eccBit2 = 0;
        forceErr.bNextBit = FALSE;
        forceErr.eccPattern = CSL_ECC_AGGR_INJECT_PATTERN_A;
        retVal = CSL_ecc_aggrForceEDCInterconnectError(pEccAggrRegs, ramId, &forceErr);
    }

    /* Wait for parity error triggered interrupt */
    if (retVal == CSL_PASS)
    {
        printf_("\r\n\r\n Waiting for Parity Error Generated Interrupt ");
        do
        {
            CSL_ecc_aggrGetEccRamErrorStatus(pEccAggrRegs, ramId, &ramEccErrorStatus);
            if (ramEccErrorStatus.parityErrorCount != 0)
            {
                gMsmcMemParityInterrupt = TRUE;
            }
            uint32_t timeOutCnt = 0;
            /* dummy wait for the interrupt */
            Osal_delay(10);
            timeOutCnt += 10;
            if (timeOutCnt > maxTimeOutMilliSeconds)
            {
                retVal = CSL_EFAIL;
                break;
            }
        } while (gMsmcMemParityInterrupt == FALSE);

        if (retVal == CSL_PASS)
        {
            printf_("\r\n\r\n  Got it");
        }
        else
        {
            printf_("\r\n\r\n  Timeout waiting for the interrupt");
        }

        printf_("\r\n\r\n Checking for the Parity Error Group ");

        if (retVal == CSL_PASS)
        {
            retVal = CSL_ecc_aggrGetEDCInterconnectErrorStatus(pEccAggrRegs, ramId, &errStatusInfo);
            if (errStatusInfo.eccGroup != MSMC_ECC_PARITY_ECC_GROUP_ACCESS_SEL)
            {
                retVal = CSL_EFAIL;
            }
            printf_("\r\n\r\n DONE");
        }
        printf_("\r\n\r\n**** MSMC Memory Parity Error Test Complete ****");
        printf_("\r\n\r\n");
    }

    return (retVal);
}
