#ifndef APP_ECC_MSMC_H
#define APP_ECC_MSMC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <ti/csl/soc.h>

#include <ti/csl/csl_ecc_aggr.h>
#include <ti/csl/soc/cslr_soc_ecc_aggr.h>


/* Test bit locations for introducing single and double bit errors */
#define MSMC_ECC_ERR_BIT_1                       (2u)
#define MSMC_ECC_ERR_BIT_2                       (8u)
#define MSMC_ECC_PARITY_ECC_GROUP_ACCESS_SEL     (3u)

/* Below Memory types for ECC are supported in the test
 * Please update the MAX Entries accordingly */
#define APP_ECC_MEMTYPE_MSMC                     (0u)
#define APP_ECC_MEMTYPE_MAX                      (APP_ECC_MEMTYPE_MSMC)

/* Max entries based on max mem type */
#if !defined(APP_ECC_AGGREGATOR_MAX_ENTRIES)
#define APP_ECC_AGGREGATOR_MAX_ENTRIES (APP_ECC_MEMTYPE_MAX + 1u)
#endif


/* MSMC RAM ID for Bank 1 is 78 : MSMC - rmw1_queue_busecc_0
   This test demonstrates the MSMC ECC using Bank 1
*/
/* TODO: Update macro with CSLR define after the CSLR bugfix for ECC aggregators on J721S2.*/
#define CC_MSMC_WRAP_ECC_AGGR0_MSMC_DATA_RAM_ID      (CSL_COMPUTE_CLUSTER0_RMW0_QUEUE_BUSECC_0_RAM_ID)



#ifdef __cplusplus
}
#endif

#endif
