/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include <string.h>
#include <ti/csl/cslr_device.h>
#include "board/board.h"
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/src/UART_osal.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/uart/soc/UART_soc.h>
#include <ti/csl/tistdtypes.h>
#include <ti/csl/arch/csl_arch.h>
#include <ti/csl/src/ip/rat/V0/csl_rat.h>

#include "ti/boot/soc/k3/sbl_slave_core_boot.h"
#include "ti/boot/soc/k3/sbl_sci_client.h"
#include "ti/boot/sbl_ver.h"
#include "ti/boot/soc/sbl_soc.h"
#include "ti/boot/soc/k3/sbl_soc_cfg.h"
#include "ti/boot/soc/k3/sbl_log.h"
#include "ti/boot/soc/k3/sbl_profile.h"

/* If BUILD_XIP is defined then SBL call SBL_enableXIPMode() to update global variable isXIPEnable */
#if defined(BUILD_XIP)
#include <ti/boot/sbl/src/ospi/sbl_ospi.h>
#endif

#if defined(SBL_ENABLE_HLOS_BOOT)
#if defined(SOC_J721E)
#include <ti/board/src/j721e_evm/include/board_utils.h>
#elif defined(SOC_J7200)
#include <ti/board/src/j7200_evm/include/board_utils.h>
#elif defined(SOC_J721S2)
#include <ti/board/src/j721s2_evm/include/board_utils.h>
#elif defined(SOC_J784S4)
#include <ti/board/src/j784s4_evm/include/board_utils.h>
#endif
#endif

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/**
 *  \brief  CSL Reset Vectors. 
 *
 *
 *  \param  None
 *
 *  \return None
 *
 */
void _resetvectors (void);
