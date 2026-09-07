/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       rat_test.c
 *@author     LiuRui
 *@date       2024.07.11
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.07.11  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include <ti/csl/csl_rat.h>
#include "rat_test.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/*===========================================================================*/
/*                        Test Function definitions                          */
/*===========================================================================*/

int32_t r5_rat_test(void)
{
    /* Declarations of variables */
    int32_t ret = 0;
    CSL_ratRegs *pRatRegs = (CSL_ratRegs *)CSL_ARMSS_RAT_CFG_BASE;

    CSL_RatTranslationCfgInfo translationCfg;

    /* Add RAT configuration to access address > 32bit address range */
    translationCfg.translatedAddress = 0x4D80800000;
    translationCfg.sizeInBytes = 0x40000;
    translationCfg.baseAddress = (uint32_t)CSL_ARMSS_RAT_REGION1_BASE;


    /* Set up RAT translation */
    CSL_ratEnableRegionTranslation(pRatRegs, 2);
    if (CSL_ratConfigRegionTranslation(pRatRegs, 2, &translationCfg) == false)
    {
        UART_printf("CSL_ratConfigRegionTranslation: failure on line no. %d \n", __LINE__);
        ret = -1;
    }

    return (ret);
}
