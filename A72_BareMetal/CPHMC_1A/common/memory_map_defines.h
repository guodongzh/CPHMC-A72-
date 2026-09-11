#ifndef MEMORY_MAP_DEFINES_H
#define MEMORY_MAP_DEFINES_H

#define DDR0_RESERVED_START    0x80000000 /*a72 u-boot */
#define DDR0_RESERVED_SIZE     0x20000000 /* 512MB */

#define DDR0_ALLOCATED_START   0xA0000000 /* DDR0_RESERVED_START + DDR0_RESERVED_SIZE */

/*------------------------------------------------*/
/* Size of various Memory Locations for each core */
/*------------------------------------------------*/
#define IPC_DATA_SIZE          0x00100000 /*  1MB */
#define RES_DATA_SIZE          0x00100000 /*  1MB */
#define DDR_SPACE_SIZE         0x00e00000 /* 14MB */

/* 14MB (IPC_DATA_SIZE + R5F_MEM_TEXT_SIZE + R5F_MEM_DATA_SIZE + DDR_SPACE_SIZE) */
#define CORE_TOTAL_SIZE        (IPC_DATA_SIZE + RES_DATA_SIZE + DDR_SPACE_SIZE)

/*-----------------------------*/
/* Start address for each core */
/*-----------------------------*/

#define MCU1_0_ALLOCATED_START DDR0_ALLOCATED_START
#define MCU1_1_ALLOCATED_START MCU1_0_ALLOCATED_START + CORE_TOTAL_SIZE
#define MCU2_0_ALLOCATED_START MCU1_1_ALLOCATED_START + CORE_TOTAL_SIZE
#define MCU2_1_ALLOCATED_START MCU2_0_ALLOCATED_START + CORE_TOTAL_SIZE
#define MCU3_0_ALLOCATED_START MCU2_1_ALLOCATED_START + CORE_TOTAL_SIZE
#define MCU3_1_ALLOCATED_START MCU3_0_ALLOCATED_START + CORE_TOTAL_SIZE
#define C66x1_ALLOCATED_START  MCU3_1_ALLOCATED_START + CORE_TOTAL_SIZE
#define C66x2_ALLOCATED_START  C66x1_ALLOCATED_START + CORE_TOTAL_SIZE
#define C7x_1_ALLOCATED_START  C66x2_ALLOCATED_START + CORE_TOTAL_SIZE

/*--------------------------- MCU R5FSS0 CORE0 --------------------------*/
#define MCU1_0_IPC_DATA_BASE   MCU1_0_ALLOCATED_START
#define MCU1_0_RES_DATA_BASE   MCU1_0_IPC_DATA_BASE + IPC_DATA_SIZE
#define MCU1_0_DDR_SPACE_BASE  MCU1_0_RES_DATA_BASE + RES_DATA_SIZE
/*--------------------------- MCU R5FSS0 CORE1 --------------------------*/
#define MCU1_1_IPC_DATA_BASE   MCU1_1_ALLOCATED_START
#define MCU1_1_RES_DATA_BASE   MCU1_1_IPC_DATA_BASE + IPC_DATA_SIZE
#define MCU1_1_DDR_SPACE_BASE  MCU1_1_RES_DATA_BASE + RES_DATA_SIZE
/*--------------------------- MAIN R5FSS0 CORE0 -------------------------*/
#define MCU2_0_IPC_DATA_BASE   MCU2_0_ALLOCATED_START
#define MCU2_0_RES_DATA_BASE   MCU2_0_IPC_DATA_BASE + IPC_DATA_SIZE
#define MCU2_0_DDR_SPACE_BASE  MCU2_0_RES_DATA_BASE + RES_DATA_SIZE
/*--------------------------- MAIN R5FSS0 CORE1 -------------------------*/
#define MCU2_1_IPC_DATA_BASE   MCU2_1_ALLOCATED_START
#define MCU2_1_RES_DATA_BASE   MCU2_1_IPC_DATA_BASE + IPC_DATA_SIZE
#define MCU2_1_DDR_SPACE_BASE  MCU2_1_RES_DATA_BASE + RES_DATA_SIZE
/*--------------------------- MAIN R5FSS1 CORE0 -------------------------*/
#define MCU3_0_IPC_DATA_BASE   MCU3_0_ALLOCATED_START
#define MCU3_0_RES_DATA_BASE   MCU3_0_IPC_DATA_BASE + IPC_DATA_SIZE
#define MCU3_0_DDR_SPACE_BASE  MCU3_0_RES_DATA_BASE + RES_DATA_SIZE
/*--------------------------- MAIN R5FSS1 CORE1 -------------------------*/
#define MCU3_1_IPC_DATA_BASE   MCU3_1_ALLOCATED_START
#define MCU3_1_RES_DATA_BASE   MCU3_1_IPC_DATA_BASE + IPC_DATA_SIZE
#define MCU3_1_DDR_SPACE_BASE  MCU3_1_RES_DATA_BASE + RES_DATA_SIZE
/*------------------------------------------------------------------------/
 * NOTE: C66x Cores IPC_DATA is swapped each other, for proper caching!! *
 * This is to overcome the limitation - "16 MB" being the minimum cache  *
 * block size in C66x. Requirement is to disable cache for IPC_DATA only *
 * i.e, the "first 1 MB" allocated for a core.                           *
 * To handle this,                                                       *
 * - C66x_1 marks "C66x2_ALLOCATED_START" + 16 MB as Cache Disabled.     *
 * - Places C66x1_IPC_DATA in "C66x2_ALLOCATED_START" + 1 MB             *
 *   - which is un-cached region from C66x_1's perspective               *
 * - Places rest of the core sections (EXT_DATA/MEM_TEXT/MEM_DATA/DDR)   *
 *   in its own dedicated regions.                                       *
 *   - i.e. starting from C66x1_ALLOCATED_START + 1 MB                   *
 *   - which remains "cached" region from C66x_1's perspective           *
 * Vice versa for C66x_2                                                 *
 * - C66x_2 marks "C66x1_ALLOCATED_START" + 16 MB as Cache Disabled.     *
 * - Places C66x2_IPC_DATA in "C66x1_ALLOCATED_START" + 1 MB             *
 *   - which is un-cached region from C66x_2's perspective               *
 * - Places rest of the core sections (EXT_DATA/MEM_TEXT/MEM_DATA/DDR)   *
 *   in its own dedicated regions.                                       *
 *   - i.e. starting from C66x1_ALLOCATED_START + 1 MB                   *
 *   - which remains "cached" region from C66x_1's perspective           */
/*--------------------------- C66x DSP CORE1 ----------------------------*/
#define C66x1_IPC_DATA_BASE    C66x2_ALLOCATED_START /* Swapped for proper caching */
#define C66x1_RES_DATA_BASE    C66x1_ALLOCATED_START + IPC_DATA_SIZE
#define C66x1_DDR_SPACE_BASE   C66x1_RES_DATA_BASE + RES_DATA_SIZE
/*--------------------------- C66x DSP CORE2 ----------------------------*/
#define C66x2_IPC_DATA_BASE    C66x1_ALLOCATED_START /* Swapped for proper caching */
#define C66x2_RES_DATA_BASE    C66x2_ALLOCATED_START + IPC_DATA_SIZE
#define C66x2_DDR_SPACE_BASE   C66x2_RES_DATA_BASE + RES_DATA_SIZE
/*--------------------------- C7x ---------------------------------------*/
#define C7x_1_IPC_DATA_BASE    C7x_1_ALLOCATED_START
#define C7x_1_RES_DATA_BASE    C7x_1_IPC_DATA_BASE + IPC_DATA_SIZE
#define C7x_1_DDR_SPACE_BASE   C7x_1_RES_DATA_BASE + RES_DATA_SIZE

#define IPC_VRING_SPACE_SIZE   0x2000000 /*  32MB */
#define IPC_VRING_SPACE_START  (C7x_1_DDR_SPACE_BASE + CORE_TOTAL_SIZE + 0xE00000)

/* PCIE IB MEME */
#define PCIE0_IB_SIZE          0x100000 /* 1MB */
#define PCIE0_IB_START         (IPC_VRING_SPACE_START + IPC_VRING_SPACE_SIZE)
#define PCIE0_IB_BASE          PCIE0_IB_START

/* PCIE IB MEME */
#define PCIE2_IB_SIZE          0x100000 /* 1MB */
#define PCIE2_IB_START         (PCIE0_IB_BASE + PCIE0_IB_SIZE)
#define PCIE2_IB_BASE          PCIE2_IB_START

/* PCIE IB MEME */
#define PCIE3_IB_SIZE          0x100000 /* 1MB */
#define PCIE3_IB_START         (PCIE2_IB_BASE + PCIE2_IB_SIZE)
#define PCIE3_IB_BASE          PCIE3_IB_START

/* PSCODE VAR*/
#define PSCODE_VAR_SIZE        0x400000 /* 4MB */
#define PSCODE_VAR_START       (PCIE3_IB_BASE + PCIE3_IB_SIZE)
#define R0_PSCODE_VAR_BASE     PSCODE_VAR_START
#define R1_PSCODE_VAR_BASE     (R0_PSCODE_VAR_BASE + PSCODE_VAR_SIZE)
#define R2_PSCODE_VAR_BASE     (R1_PSCODE_VAR_BASE + PSCODE_VAR_SIZE)
#define R3_PSCODE_VAR_BASE     (R2_PSCODE_VAR_BASE + PSCODE_VAR_SIZE)
#define C60_PSCODE_VAR_BASE    (R3_PSCODE_VAR_BASE + PSCODE_VAR_SIZE)
#define C61_PSCODE_VAR_BASE    (C60_PSCODE_VAR_BASE + PSCODE_VAR_SIZE)
#define C70_PSCODE_VAR_BASE    (C61_PSCODE_VAR_BASE + PSCODE_VAR_SIZE)
#define MCU_R1_PSCODE_VAR_BASE  (C70_PSCODE_VAR_BASE + PSCODE_VAR_SIZE)

/* fw file */
#define FW_DATA_SIZE           0x200000                                /* 2MB */
#define FW_START               (MCU_R1_PSCODE_VAR_BASE + PSCODE_VAR_SIZE) /* 0xafc00000 */
#define FW_BASE                FW_START

/* fast ipc */
#define FAST_DATA_SIZE         0x1000 /* 4KB */
#define FAST_DATA_START        (FW_BASE + FW_DATA_SIZE)
#define R0_FAST_DATA_BASE      FAST_DATA_START
#define R1_FAST_DATA_BASE      (R0_FAST_DATA_BASE + FAST_DATA_SIZE)
#define R2_FAST_DATA_BASE      (R1_FAST_DATA_BASE + FAST_DATA_SIZE)
#define R3_FAST_DATA_BASE      (R2_FAST_DATA_BASE + FAST_DATA_SIZE)
#define C60_FAST_DATA_BASE     (R3_FAST_DATA_BASE + FAST_DATA_SIZE)
#define C61_FAST_DATA_BASE     (C60_FAST_DATA_BASE + FAST_DATA_SIZE)
#define C70_FAST_DATA_BASE     (C61_FAST_DATA_BASE + FAST_DATA_SIZE)

/* scada */
#define SCADA_DATA_SIZE        0xc00   /* 3KB */
#define CORE_4RMT_TOTAL_SIZE   0x40000 /* 256KB */
#define CORE0_SCADA_CFG_START  (C70_FAST_DATA_BASE + FAST_DATA_SIZE)
#define CORE1_SCADA_CFG_START  (CORE0_SCADA_CFG_START + CORE_4RMT_TOTAL_SIZE)
#define CORE2_SCADA_CFG_START  (CORE1_SCADA_CFG_START + CORE_4RMT_TOTAL_SIZE)
#define CORE3_SCADA_CFG_START  (CORE2_SCADA_CFG_START + CORE_4RMT_TOTAL_SIZE)
#define CORE4_SCADA_CFG_START  (CORE3_SCADA_CFG_START + CORE_4RMT_TOTAL_SIZE)
#define CORE5_SCADA_CFG_START  (CORE4_SCADA_CFG_START + CORE_4RMT_TOTAL_SIZE)
#define CORE6_SCADA_CFG_START  (CORE5_SCADA_CFG_START + CORE_4RMT_TOTAL_SIZE)

#if (Core == 0)
#define SCADA_CFG_START CORE0_SCADA_CFG_START

#elif (Core == 1)
#define SCADA_CFG_START CORE1_SCADA_CFG_START

#elif (Core == 2)
#define SCADA_CFG_START CORE2_SCADA_CFG_START

#elif (Core == 3)
#define SCADA_CFG_START CORE3_SCADA_CFG_START

#elif (Core == 4)
#define SCADA_CFG_START CORE4_SCADA_CFG_START

#elif (Core == 5)
#define SCADA_CFG_START CORE5_SCADA_CFG_START

#elif (Core == 6)
#define SCADA_CFG_START CORE6_SCADA_CFG_START

#endif

#define SCADA_CFG_BASE            SCADA_CFG_START
#define YX_DATA_BASE              (SCADA_CFG_BASE + SCADA_DATA_SIZE)
#define YK_DATA_BASE              (YX_DATA_BASE + SCADA_DATA_SIZE)
#define YC_DATA_BASE              (YK_DATA_BASE + SCADA_DATA_SIZE)
#define YT_DATA_BASE              (YC_DATA_BASE + SCADA_DATA_SIZE)

/* TFR MEM */
#define TFR_MEM_SIZE              0x2000000 /* 32MB */
#define TFR_MEM_START             (CORE6_SCADA_CFG_START + CORE_4RMT_TOTAL_SIZE)
#define R0_TFR_MEM_BASE           TFR_MEM_START
#define R1_TFR_MEM_BASE           (R0_TFR_MEM_BASE + TFR_MEM_SIZE)
#define R2_TFR_MEM_BASE           (R1_TFR_MEM_BASE + TFR_MEM_SIZE)
#define R3_TFR_MEM_BASE           (R2_TFR_MEM_BASE + TFR_MEM_SIZE)
#define C60_TFR_MEM_BASE          (R3_TFR_MEM_BASE + TFR_MEM_SIZE)
#define C61_TFR_MEM_BASE          (C60_TFR_MEM_BASE + TFR_MEM_SIZE)
#define C71_TFR_MEM_BASE          (C61_TFR_MEM_BASE + TFR_MEM_SIZE)

/* PSCode watch tables */
#define PSCode_WATCH_TAB_SIZE     0x2000                            /* 8KB */
#define PSCode_WATCH_TAB_START    (C71_TFR_MEM_BASE + TFR_MEM_SIZE) /*  */
#define PSCode_WATCH_TAB_BASE     PSCode_WATCH_TAB_START

/* PSCode watch data */
#define PSCode_WATCH_DATA_SIZE     0x2000                            /* 8KB */
#define PSCode_WATCH_DATA_START    (PSCode_WATCH_TAB_BASE + PSCode_WATCH_TAB_SIZE) /*  */
#define PSCode_WATCH_DATA_BASE     PSCode_WATCH_DATA_START

/* PSCode force tables */
#define PSCode_VARS_MEM_SIZE      0x6000                            /* 24KB */
#define PSCode_VARS_MEM_START     (PSCode_WATCH_DATA_BASE + PSCode_WATCH_DATA_SIZE) /*  */
#define PSCode_VARS_MEM_BASE      PSCode_VARS_MEM_START

/* PSCode var tables */
#define PSCode_VAR_MEM_SIZE       0x2000                                        /* 8KB */
#define PSCode_VAR_MEM_START      (PSCode_VARS_MEM_BASE + PSCode_VARS_MEM_SIZE) /*  */
#define PSCode_VAR_MEM_BASE       PSCode_VAR_MEM_START

/* PSCode res version */
#define PSCode_RES_VER_SIZE       0x1000                                        /* 4KB */
#define PSCode_RES_VER_START      (PSCode_VAR_MEM_BASE + PSCode_VAR_MEM_SIZE)  /*  */
#define PSCode_RES_VER_BASE       PSCode_RES_VER_START

/* init flag  */
#define INIT_FLAG_SIZE            0x1000 /* 4KB */
#define INIT_FLGA_START           (PSCode_RES_VER_BASE + PSCode_RES_VER_SIZE)
#define INIT_FLGA_BASE            INIT_FLGA_START

/* ft3 tx cfg  */
#define FT3_TX_CFG_SIZE           0x4000 /* 16KB */
#define FT3_TX_CFG_START          (INIT_FLGA_BASE + INIT_FLAG_SIZE)
#define FT3_TX_CFG_BASE           FT3_TX_CFG_START

/* ft3 rx cfg  */
#define FT3_RX_CFG_SIZE           0x4000 /* 16KB */
#define FT3_RX_CFG_START          (FT3_TX_CFG_BASE + FT3_TX_CFG_SIZE)
#define FT3_RX_CFG_BASE           FT3_RX_CFG_START

/* squ_mod_cfg  */
#define SQU_MOD_CFG_SIZE           0x1000 /* 4KB */
#define SQU_MOD_CFG_START          (FT3_RX_CFG_BASE + FT3_RX_CFG_SIZE)
#define SQU_MOD_CFG_BASE           SQU_MOD_CFG_START

/* ETH SHM INFO */
#define ETH_INFO_MEM_SIZE         0x10000
#define ETH_INFO_MEM_START        (SQU_MOD_CFG_BASE + SQU_MOD_CFG_SIZE) /*  */
#define ETH_INFO_MEM_BASE         ETH_INFO_MEM_START

/* ETH RX QUE */
#define ETH_RX_QUE_MEM_SIZE       0x10000
#define ETH_RX_QUE_MEM_START      (ETH_INFO_MEM_BASE + ETH_INFO_MEM_SIZE) /*  */
#define ETH_RX_QUE_MEM_BASE       ETH_RX_QUE_MEM_START

/* ETH TX QUE */
#define R5_ETH_TX_BUF_SIZE        0x1000
#define A72_ETH_TX_BUF_SIZE       0x10000
#define R5_ETH_TX_BUF_START       (ETH_RX_QUE_MEM_BASE + ETH_RX_QUE_MEM_SIZE)

#define R1_TX_R0_ETH_MEM_BASE     R5_ETH_TX_BUF_START
#define R2_TX_R0_ETH_MEM_BASE     (R1_TX_R0_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE * 2)
#define R3_TX_R0_ETH_MEM_BASE     (R2_TX_R0_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE * 2)
#define A72_TX_R0_ETH_MEM_BASE    (R3_TX_R0_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE * 2)
#define R0_MULTICAST_ETH_MEM_BASE (A72_TX_R0_ETH_MEM_BASE + A72_ETH_TX_BUF_SIZE)

#define R0_TX_R1_ETH_MEM_BASE     (R0_MULTICAST_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R2_TX_R1_ETH_MEM_BASE     (R0_TX_R1_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R3_TX_R1_ETH_MEM_BASE     (R2_TX_R1_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R1_MULTICAST_ETH_MEM_BASE (R3_TX_R1_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)

#define R0_TX_R2_ETH_MEM_BASE     (R1_MULTICAST_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R1_TX_R2_ETH_MEM_BASE     (R0_TX_R2_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R3_TX_R2_ETH_MEM_BASE     (R1_TX_R2_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R2_MULTICAST_ETH_MEM_BASE (R3_TX_R2_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)

#define R0_TX_R3_ETH_MEM_BASE     (R2_MULTICAST_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R1_TX_R3_ETH_MEM_BASE     (R0_TX_R3_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R2_TX_R3_ETH_MEM_BASE     (R1_TX_R3_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define R3_MULTICAST_ETH_MEM_BASE (R2_TX_R3_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)

#define VERSION_SIZE              0x1000
#define VERSION_START             (R3_MULTICAST_ETH_MEM_BASE + R5_ETH_TX_BUF_SIZE)
#define VERSION_BASE              VERSION_START

#define IRIGB_SIZE              0x1000
#define IRIGB_START             (VERSION_BASE + VERSION_SIZE)
#define IRIGB_BASE              IRIGB_START

#define LINUX_TEST_FLAG_SIZE      0x1000
#define LINUX_TEST_FLAG_START     (IRIGB_BASE + IRIGB_SIZE)
#define LINUX_TEST_FLAG_BASE      LINUX_TEST_FLAG_START

#define LINUX_ENT_FLAG_SIZE       0x1000
#define LINUX_ENT_FLAG_START      (LINUX_TEST_FLAG_BASE + LINUX_TEST_FLAG_SIZE)
#define LINUX_ENT_FLAG_BASE       LINUX_ENT_FLAG_START

#endif  // MEMORY_MAP_DEFINES_H
