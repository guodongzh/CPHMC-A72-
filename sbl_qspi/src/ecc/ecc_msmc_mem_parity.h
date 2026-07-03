#ifndef APP_ECC_MSMC_MEM_PARITY_H
#define APP_ECC_MSMC_MEM_PARITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/csl/csl_rat.h>
#include <ti/csl/soc.h>

#define MSMC_PARITY_TEST_SUPPORT

#if defined (MSMC_PARITY_TEST_SUPPORT)
#if defined (SOC_J721E)
#define MSMC_PARITY_ECC_AGGR_REGION_LOCAL_BASE        (0x60000000u)
#define MSMC_PARITY_ECC_AGGR_RAT_REGION_INDEX         (0)
#define MSMC_PARITY_ECC_AGGR_RAT_CFG_BASE             (CSL_MCU_ARMSS_RAT_CFG_BASE)
#else
#error "SOC is not supported for the build"
#endif
#endif

int32_t msmcEccMemParityTest(void);

#ifdef __cplusplus
}
#endif

#endif /*APP_ECC_MSMC_MEM_PARITY_H */
