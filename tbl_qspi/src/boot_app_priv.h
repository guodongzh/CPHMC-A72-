#ifndef BOOT_APP_PRIV_H_
#define BOOT_APP_PRIV_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* files related to SBL */
#include "ti/boot/src/rprc/sbl_rprc_parse.h"
#include "ti/boot/src/ospi/sbl_ospi.h"
#include "ti/boot/soc/sbl_soc.h"
#include "ti/boot/soc/k3/sbl_slave_core_boot.h"
#include "ti/boot/soc/k3/sbl_soc_cfg.h"

/* files related to CSL */
#include <ti/csl/arch/csl_arch.h>
#include <ti/csl/soc.h>
#include <ti/csl/cslr.h>

/*files related to borad */
#include "board/board.h"
#include "board/board_cfg.h"
#include "board/src/flash/include/board_flash.h"
#include "boot_core_defs.h"

/* files related to osal */
#include <ti/osal/osal.h>
#include <ti/osal/TaskP.h>

/* files related to uart */
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/drv/uart/soc/UART_soc.h>
#include <ti/drv/spi/soc/SPI_soc.h>

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

extern const sblSlaveCoreInfo_t sbl_late_slave_core_stages_info[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE];
extern uint32_t main_boot_flash_images[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE];
extern uint32_t debug_boot_flash_images[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE];
extern uint32_t back_up_boot_flash_images[NUM_BOOT_STAGES][MAX_CORES_PER_STAGE];
extern const cpu_core_id_t boot_array[6];

/* Defines boot order for the first stage of the Main Domain
 * boot sequence for J721E SOC */
extern cpu_core_id_t boot_order_first_stage[];

/* Defines boot order for the second stage of the Main Domain
 * boot sequence for J721E SOC */
extern cpu_core_id_t boot_order_second_stage[];

/* Points to boot order arrays for each of the boot stages */
extern cpu_core_id_t *boot_array_stage[];

/* Defines number of cores booted in each stage */
extern uint8_t num_cores_per_boot_stage[];

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

#if defined(MPU1_HLOS_BOOT_ENABLED)
    /* Function to clean the MCU R5 cache for a given start address and given memory size */
    void BootApp_McuDCacheClean(void *addr, uint32_t size);
#endif

#ifdef __cplusplus
}
#endif

#endif /* BOOT_APP_PRIV_H_ */
