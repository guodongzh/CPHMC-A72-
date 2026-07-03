#ifndef ECC_DDR_H_
#define ECC_DDR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/csl/soc.h>
#include <ti/csl/csl_ecc_aggr.h>
#include <ti/csl/csl_emif.h>
#include <ti/drv/uart/UART_stdio.h>

#define DDR_ECC_TEST_ADDR  (0x90000000 + 0x100)

/* Below Memory types for ECC are supported in the test
 * Please update the MAX Entries accordinly */
#define APP_ECC_MEMTYPE_DDR                      (0u)

/* Max entries based on max mem type */
#if !defined(APP_ECC_AGGREGATOR_MAX_ENTRIES)
#define APP_ECC_AGGREGATOR_MAX_ENTRIES (APP_ECC_MEMTYPE_DDR + 1u)
#endif


#define EMIF_ECC_MEM_BLOCK_SIZE      0x200
#define EMIF_ECC_DATA_SIZE_PER_BLOCK 0x40


/* ESM Base Addresses */
#if defined(SOC_AM65XX)
#if defined(BUILD_MPU)
#define ESM_CFG_BASE    (CSL_ESM0_CFG_BASE)
#define ESM_LO_INT      (CSL_GIC0_INTR_ESM0_BUS_ESM_INT_LOW_LVL)
#define ESM_HI_INT      (CSL_GIC0_INTR_ESM0_BUS_ESM_INT_HI_LVL)
#define ESM_CFG_ERR_INT (CSL_GIC0_INTR_ESM0_BUS_ESM_INT_CFG_LVL)
#endif
#endif

int32_t DDREccTest(void);
uintptr_t DDRGetTranslatedAddress(uintptr_t nonECCAddress);

int ecc_test(void);

#ifdef __cplusplus
}
#endif

#endif /* ECC_DDR_H_ */
