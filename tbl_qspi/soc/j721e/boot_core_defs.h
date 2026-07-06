/**
 *  \file     boot_core_defs.h
 *
 *  \brief    Header file for slave boot core definitions
 *
 */

#ifdef __cplusplus
extern "C"
{
#endif

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* Number of boot stages for the main domain multi-stage boot sequence */
#define NUM_BOOT_STAGES     2

/* Maximum number of boot cores per boot stage */
#define MAX_CORES_PER_STAGE   7

/* Macros representing the offset where the App Image has to be written/Read from
   the OSPI Flash.
*/

/* Location of ATF/OPTEE - used for both Linux  */
#define ATF_SPL_FLASH_ADDR (0x180000U)

/* Location of rtos app */
#define CORE0_APPS_FLASH_ADDR     (0x280000)
#define CORE1_APPS_FLASH_ADDR     (0x380000)
#define CORE2_APPS_FLASH_ADDR     (0x480000)
#define CORE3_APPS_FLASH_ADDR     (0x580000)
#define CORE4_APPS_FLASH_ADDR     (0x680000)
#define CORE5_APPS_FLASH_ADDR     (0x780000)
#define CORE6_APPS_FLASH_ADDR     (0x880000)
#define ALL_CORES_APPS_FLASH_ADDR (0xa80000)

#define CORE0_APPS_NULL_FLASH_ADDR     (0xc80000)
#define CORE1_APPS_NULL_FLASH_ADDR     (0xc82000)
#define CORE2_APPS_NULL_FLASH_ADDR     (0xc84000)
#define CORE3_APPS_NULL_FLASH_ADDR     (0xc86000)
#define CORE4_APPS_NULL_FLASH_ADDR     (0xc88000)
#define CORE5_APPS_NULL_FLASH_ADDR     (0xc8a000)
#define CORE6_APPS_NULL_FLASH_ADDR     (0xc8c000)

/* Location Address used as flag to indicate loading of
 * all HLOS appimages for OSPI */
#define MAIN_DOMAIN_HLOS (0x1)

/* Important RAM address macros */
#define ATF_START_RAM_ADDR (0x70000000)

/* this whole structure fits inside sblbootbuff segment in memory */
#define SBL_MAX_BOOT_BUFF_SIZE (0x4000000 - 0x10)

#ifdef __cplusplus
}
#endif
