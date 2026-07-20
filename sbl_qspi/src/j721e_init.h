/**
* \file  j721e_init.h
*
* \brief Public API for J721E SoC early initialization.
*
* Translates the following GEL functions to C for use in bare-metal /
* RTOS drivers:
*   - Configure_ATCM()              (J721E.gel)
*   - MCU_R5_Cluster_0_split()      (J721E_R5LOCKSTEP.gel)
*   - Set_All_PLL()                 (J721E_PLL_OFC1.gel)
*   - Set_PSC_All_On()              (J721E_PSC.gel)
*
* Usage from your driver:
*
*   #include "j721e_init.h"
*
*   void my_boot_sequence(void) {
*       j721e_early_init();   // runs all 4 steps in GEL OnTargetConnect order
*   }
*
* Or call individually:
*   Configure_ATCM();
*   MCU_R5_Cluster_0_split();
*   Set_All_PLL();
*   Set_PSC_All_On();
*/

#ifndef J721E_INIT_H
#define J721E_INIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* Public API                                                                 */
/* ========================================================================= */

/**
* \brief  Configure ATCM (Tightly-Coupled Memory A) for all 6 R5F cores.
*
* Writes 0x888 to ATCM config registers on:
*   - MCU R5F Core 0 & Core 1
*   - MAIN R5F Cluster 0 Core 0 & Core 1
*   - MAIN R5F Cluster 1 Core 0 & Core 1
*
* Must be called from the DMSC Cortex-M3 or after the RAT is configured.
*/
void Configure_ATCM(void);

/**
* \brief  Put MCU R5F Cluster 0 into split (dual-core independent) mode.
*
* Writes 0x08 to MCU_SEC_MMR cluster control register offset 0x40.
* 0x08 = split mode, 0x09 = lockstep mode.
*/
void MCU_R5_Cluster_0_split(void);

/**
* \brief  Program all 22 PLLs to OFC1 operating points.
*
* Programs:
*   - 19 Main domain PLLs (PLL0-8, PLL12-19, PLL23, PLL25)
*   -  3 MCU  domain PLLs (MCU_PLL0-2)
*
* Internally calls turn_on_lpsc_wkupmcu2main() to enable the
* WKUP→MAIN bridge before touching Main domain registers.
*
* \note  This function polls PLL lock bits and may block for
*        several milliseconds during PLL lock acquisition.
*/
void Set_All_PLL(void);

/**
* \brief  Power up all PSC modules across WKUP and MAIN domains.
*
* Iterates over ~107 LPSC modules, setting each power domain ON
* and module state to ENABLE.
*
* \return 1 = all modules powered successfully, 0 = at least one failure.
*/
int Set_PSC_All_On(void);

/**
* \brief  Convenience: run all 4 early-init steps in GEL order.
*
* Equivalent to:
*   1. Configure_ATCM()
*   2. MCU_R5_Cluster_0_split()
*   3. Set_All_PLL()
*   4. Set_PSC_All_On()
*/
void j721e_early_init(void);

#ifdef __cplusplus
}
#endif

#endif /* J721E_INIT_H */
