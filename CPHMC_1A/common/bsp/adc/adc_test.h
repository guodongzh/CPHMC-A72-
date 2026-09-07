/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       adc_test.h
*@author     xqb
*@date       2024.07.21
*@brief      adc module test source.h file
*@par        History
*Date        Version   Author     Description
2024.07.21   1.0       xqb        example
******************************************************************************/
#ifndef __ADC_TEST_H_
#define __ADC_TEST_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ti/csl/csl_types.h>
#include <ti/csl/soc.h>
#include <ti/csl/hw_types.h>
#include <ti/csl/arch/csl_arch.h>
#include <ti/csl/src/ip/adc/V0/adc.h>
#include "debug_config.h"
#include <string.h>
#include <ti/csl/soc.h>
#include <ti/drv/udma/udma.h>
#include <ti/osal/osal.h>
#include <ti/osal/TaskP.h>
#include "udma_apputils.h"

/* ========================================================================== */
/*                                Macros                                      */
/* ========================================================================== */
#if defined (SOC_TDA3XX) || defined (SOC_DRA78x)
#define APP_ADC_MODULE          (SOC_TSC_ADC_BASE)
#elif defined (SOC_AM65XX) || defined (SOC_J721E) || defined (SOC_J7200)
#define APP_ADC_MODULE          (CSL_MCU_ADC0_BASE)
#else
#define APP_ADC_MODULE          (CSL_ADC0_BASE)
#endif
#define APP_ADC_DIV             (1U)
/* Reference voltage for ADC - should be given in mV */
#define APP_ADC_REF_VOLTAGE     (1800U)
#if defined (SOC_TDA3XX) || defined (SOC_DRA78x)
#define APP_ADC_RANGE_MAX       (1024U)
#elif defined (SOC_AM65XX) || defined (SOC_J721E) || defined (SOC_J7200) || defined (SOC_AM64X)
#define APP_ADC_RANGE_MAX       (4096U)
#endif

#define APP_ADC_INT_M4                                  (34U)
#define APP_ADC_INT_DSP                                 (32U)
#if ((__ARM_ARCH == 7) && (__ARM_ARCH_PROFILE == 'M') && defined(__ARM_FEATURE_SIMD32))
    #define APP_ADC_INT                     (APP_ADC_INT_M4)
    #define EDMA3_CC_REGION                 (EDMA3_CC_REGION_M4)
    #define XBAR_CPU                        (CSL_XBAR_IRQ_CPU_ID_IPU1)
    #define XBAR_INST                       (CSL_XBAR_INST_IPU1_IRQ_34)
    #define XBAR_INTR_SOURCE                (CSL_XBAR_TSC_ADC_IRQ_GENINT)
#elif defined (_TMS320C6X)
    #define APP_ADC_INT                     (APP_ADC_INT_DSP)
    #define XBAR_CPU                        (CSL_XBAR_IRQ_CPU_ID_DSP1)
    #define XBAR_INST                       (CSL_XBAR_INST_DSP1_IRQ_32)
    #define XBAR_INTR_SOURCE                (CSL_XBAR_TSC_ADC_IRQ_GENINT)
#endif

#define CONFIG_ADC0_BASE_ADDR CSL_MCU_ADC0_BASE


static void ADCModuleEnable(void);
void ADC_Test(void);
static void ADCStartConfig(void);
static void ADCStopConfig(void);
#endif /* __ADC_TEST_H_ */
