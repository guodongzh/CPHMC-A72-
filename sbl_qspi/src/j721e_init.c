/**
 * \file  j721e_init_mcu1_0.c
 *
 * \brief J721E SoC initialisation — MCU1_0 (R5F) variant.
 *
 * Runs on MCU1_0 with full 32-bit address space.  All registers are
 * accessed at their native SoC global physical addresses; no RAT
 * translation is needed (unlike the DMSC Cortex-M3 version).
 *
 * Original GEL sources translated:
 *   J721E.gel                              → Configure_ATCM()
 *   J721E_R5LOCKSTEP.gel                   → MCU_R5_Cluster_0_split()
 *   J721E_PLL/J721E_PLL_OFC1.gel           → Set_All_PLL()
 *   J721E_PLL/J721E_PLL_PARAMS_OFC1.gel    → (all PLL divider constants)
 *   J721E_PSC/J721E_PSC.gel                → Set_PSC_All_On()
 *
 * Every PLL divider value has been verified line-by-line against
 * J721E_PLL_PARAMS_OFC1.gel (OFC1 operating point).
 */

#include <stdint.h>
#include "j721e_init.h"
#include "ti/osal/src/printf.h"

/* =========================================================================
 * SoC base addresses (native physical, no RAT offset)
 * ========================================================================= */
#define CSL_MCU_SEC_MMR0_CFG0_BASE       (0x45A50000U)
#define CSL_MAIN_SEC_MMR0_BOOT_CTRL_BASE (0x45A40000U)

/* PLL base addresses */
#define CSL_PLL0_CFG_BASE                (0x00680000U)
#define CSL_MCU_PLL0_CFG_BASE            (0x40D00000U)

/* =========================================================================
 * Power Domain / LPSC indices
 * ========================================================================= */

/*
 * macro definitions for the WKUP_PSC
 */

// PD indexes
#define PD_GP_CORE_CTL_WKUP              0
#define PD_MCU_PULSAR                    1

// LPSC indexes
#define LPSC_WKUP_ALWAYSON               0
#define LPSC_DMSC                        1  // For M3 reset control only.
#define LPSC_DEBUG2DMSC                  2
#define LPSC_WKUP_GPIO                   3  // For clock gating purposes only. Always on.
#define LPSC_WKUPMCU2MAIN                4
#define LPSC_MAIN2WKUPMCU                5
#define LPSC_MCU_TEST                    6
#define LPSC_MCU_DEBUG                   7
#define LPSC_MCU_MCAN_0                  8
#define LPSC_MCU_MCAN_1                  9
#define LPSC_MCU_OSPI_0                  10
#define LPSC_MCU_OSPI_1                  11
#define LPSC_MCU_HYPERBUS                12
#define LPSC_MCU_I3C_0                   13
#define LPSC_MCU_I3C_1                   14
#define LPSC_MCU_ADC_0                   15
#define LPSC_MCU_ADC_1                   16
#define LPSC_WKUP_SPARE_0                17
#define LPSC_WKUP_SPARE_1                18
#define LPSC_MCU_R5_0                    19  // PD_MCU_PULSAR
#define LPSC_MCU_R5_1                    20  // PD_MCU_PULSAR
#define LPSC_MCU_PULSAR_PBIST_0          21  // PD_MCU_PULSAR
// WKUP CTRL MMRs --> controls WKUP analog stuff
// MCU CTRL MMRs --> controls MCU PSRAM
// MCU PLL MMRs --> controls MCU PLL programming interfaces
// WKUP PLL Controller --> runs to wkup efuse chain storage

/*
 * macro definitions for the MAIN_PSC
 */

// PD indexes
#define PD_GP_CORE_CTRL                  0
#define PD_MCANSS                        1
#define PD_DSS                           2
#define PD_ICSS                          3
#define PD_9GSS                          4
#define PD_SERDES_0                      5
#define PD_SERDES_1                      6
#define PD_SERDES_2                      7
#define PD_SERDES_3                      8
#define PD_SERDES_4                      9
#define PD_SERDES_5                      10
#define PD_TIMER                         11
#define PD_C71X_0                        12
#define PD_C71X_1                        13
#define PD_A72_CLUSTER_0                 14
#define PD_A72_0                         15
#define PD_A72_1                         16
#define PD_A72_CLUSTER_1                 17
#define PD_A72_2                         18
#define PD_A72_3                         19
#define PD_GPUCOM                        20
#define PD_GPUCORE                       21
#define PD_C66X_0                        22
#define PD_C66X_1                        23
#define PD_PULSAR_0                      24
#define PD_PULSAR_1                      25
#define PD_DECODE                        26
#define PD_ENCODE                        27
#define PD_DMPAC                         28
#define PD_VPAC                          29

// LPSC indexes
#define LPSC_MAIN_ALWAYSON               0
#define LPSC_MAIN_TEST                   1
#define LPSC_MAIN_PBIST                  2
#define LPSC_PER_AUDIO                   3
#define LPSC_PER_ATL                     4
#define LPSC_PER_MLB                     5
#define LPSC_PER_MOTOR                   6
#define LPSC_PER_MISCIO                  7
#define LPSC_PER_GPMC                    8
#define LPSC_PER_VPFE                    9
#define LPSC_PER_VPE                     10
#define LPSC_PER_SPARE_0                 11
#define LPSC_PER_SPARE_1                 12
#define LPSC_MAIN_DEBUG                  13
#define LPSC_EMIF_DATA_0                 14
#define LPSC_EMIF_CFG_0                  15
#define LPSC_EMIF_DATA_1                 16
#define LPSC_EMIF_CFG_1                  17
#define LPSC_PER_SPARE_2                 18
#define LPSC_CC_TOP_PBIST                19
#define LPSC_USB_0                       20
#define LPSC_USB_1                       21
#define LPSC_USB_2                       22
#define LPSC_MMC4B_0                     23
#define LPSC_MMC4B_1                     24
#define LPSC_MMC8B_0                     25
#define LPSC_UFS_0                       26
#define LPSC_UFS_1                       27
#define LPSC_PCIE_0                      28
#define LPSC_PCIE_1                      29
#define LPSC_PCIE_2                      30
#define LPSC_PCIE_3                      31
#define LPSC_SAUL                        32
#define LPSC_PER_I3C                     33
#define LPSC_MAIN_MCANSS_0               34
#define LPSC_MAIN_MCANSS_1               35
#define LPSC_MAIN_MCANSS_2               36
#define LPSC_MAIN_MCANSS_3               37
#define LPSC_MAIN_MCANSS_4               38
#define LPSC_MAIN_MCANSS_5               39
#define LPSC_MAIN_MCANSS_6               40
#define LPSC_MAIN_MCANSS_7               41
#define LPSC_MAIN_MCANSS_8               42
#define LPSC_MAIN_MCANSS_9               43
#define LPSC_MAIN_MCANSS_10              44
#define LPSC_MAIN_MCANSS_11              45
#define LPSC_MAIN_MCANSS_12              46
#define LPSC_MAIN_MCANSS_13              47
#define LPSC_DSS                         48
#define LPSC_DSS_PBIST                   49
#define LPSC_DSI                         50
#define LPSC_EDP_0                       51
#define LPSC_EDP_1                       52
#define LPSC_CSIRX_0                     53
#define LPSC_CSIRX_1                     54
#define LPSC_CSIRX_2                     55
#define LPSC_CSITX_0                     56
#define LPSC_TX_DPHY_0                   57
#define LPSC_CSIRX_PHY_0                 58
#define LPSC_CSIRX_PHY_1                 59
#define LPSC_CSIRX_PHY_2                 60
#define LPSC_ICSSG_0                     61
#define LPSC_ICSSG_1                     62
#define LPSC_9GSS                        63
#define LPSC_SERDES_0                    64
#define LPSC_SERDES_1                    65
#define LPSC_SERDES_2                    66
#define LPSC_SERDES_3                    67
#define LPSC_SERDES_4                    68
#define LPSC_SERDES_5                    69
#define LPSC_DMTIMER_0                   70
#define LPSC_DMTIMER_1                   71
#define LPSC_DMTIMER_2                   72
#define LPSC_DMTIMER_3                   73
#define LPSC_C71X_0                      74
#define LPSC_C71X_0_PBIST                75
#define LPSC_C71X_1                      76
#define LPSC_C71X_1_PBIST                77
#define LPSC_A72_CLUSTER_0               78
#define LPSC_A72_CLUSTER_0_PBIST         79
#define LPSC_A72_0                       80
#define LPSC_A72_1                       81
#define LPSC_A72_CLUSTER_1               82
#define LPSC_A72_CLUSTER_1_PBIST         83
#define LPSC_A72_2                       84
#define LPSC_A72_3                       85
#define LPSC_GPUCOM                      86
#define LPSC_GPUPBIST                    87
#define LPSC_GPUCORE                     88
#define LPSC_C66X_0                      89
#define LPSC_C66X_PBIST_0                90
#define LPSC_C66X_1                      91
#define LPSC_C66X_PBIST_1                92
#define LPSC_PULSAR_0_R5_0               93
#define LPSC_PULSAR_0_R5_1               94
#define LPSC_PULSAR_0_PBIST              95
#define LPSC_PULSAR_1_R5_0               96
#define LPSC_PULSAR_1_R5_1               97
#define LPSC_PULSAR_1_PBIST              98
#define LPSC_DECODE_0                    99
#define LPSC_DECODE_PBIST                100
#define LPSC_ENCODE_0                    101
#define LPSC_ENCODE_PBIST                102
#define LPSC_DMPAC                       103
#define LPSC_SDE                         104
#define LPSC_DMPAC_PBIST                 105
#define LPSC_VPAC                        106
#define LPSC_VPAC_PBIST                  107
// Main CTRL MMRs --> not controlled by Main PSC
// Main PLL CTRL --> not controlled by Main PSC
// Main PLL MMRs --> not controlled by Main PSC

// common base addresses
#define WAKEUP_PSC_BASE                  (0x42000000)
#define MAIN_PSC_BASE                    (0x00400000)
#define PSC_PID                          (0x44827A00)
#define M3_SOC_OFFSET                    (0x60000000)
#define M3_RAT_CTRL                      (0x44200020)  // Base address + CTRL register offset.

// Defines for WKUP and MAIN PSC indices
#define MAIN_PSC_INDEX                   0
#define WAKEUP_PSC_INDEX                 1

// PSC MMR interface
#define PSC_MDCTL00                      (0xA00)
#define PSC_MDSTAT00                     (0x800)
#define PSC_PDCTL00                      (0x300)
#define PSC_PDSTAT00                     (0x200)
#define PSC_PTCMD                        (0x120)
#define PSC_PTSTAT                       (0x128)

// PSC Parameter definitions
#define PSC_PD_OFF                       (0x0)
#define PSC_PD_ON                        (0x1)

#define PSC_SYNCRESETDISABLE             (0x0)
#define PSC_SYNCRESET                    (0x1)
#define PSC_DISABLE                      (0x2)
#define PSC_ENABLE                       (0x3)

#define PSC_TIMEOUT                      (100)

// PSC Register pre-calculations
#define WAKEUP_PSC_MDCTL_BASE            (WAKEUP_PSC_BASE + PSC_MDCTL00)
#define WAKEUP_PSC_MDSTAT_BASE           (WAKEUP_PSC_BASE + PSC_MDSTAT00)
#define WAKEUP_PSC_PDCTL_BASE            (WAKEUP_PSC_BASE + PSC_PDCTL00)
#define WAKEUP_PSC_PDSTAT_BASE           (WAKEUP_PSC_BASE + PSC_PDSTAT00)
#define WAKEUP_PSC_PTCMD_BASE            (WAKEUP_PSC_BASE + PSC_PTCMD)
#define WAKEUP_PSC_PTSTAT_BASE           (WAKEUP_PSC_BASE + PSC_PTSTAT)
#define WAKEUP_PSC_PTCMD                 WAKEUP_PSC_PTCMD_BASE
#define WAKEUP_PSC_PTSTAT                WAKEUP_PSC_PTSTAT_BASE

#define MAIN_PSC_MDCTL_BASE              (MAIN_PSC_BASE + PSC_MDCTL00)
#define MAIN_PSC_MDSTAT_BASE             (MAIN_PSC_BASE + PSC_MDSTAT00)
#define MAIN_PSC_PDCTL_BASE              (MAIN_PSC_BASE + PSC_PDCTL00)
#define MAIN_PSC_PDSTAT_BASE             (MAIN_PSC_BASE + PSC_PDSTAT00)
#define MAIN_PSC_PTCMD_BASE              (MAIN_PSC_BASE + PSC_PTCMD)
#define MAIN_PSC_PTSTAT_BASE             (MAIN_PSC_BASE + PSC_PTSTAT)
#define MAIN_PSC_PTCMD                   MAIN_PSC_PTCMD_BASE
#define MAIN_PSC_PTSTAT                  MAIN_PSC_PTSTAT_BASE



#define FREF 20.0 	//20MHz HFOSC0 reference clock on QT
#define FREF_SVB 25 	//25MHz HFOSC0 reference clock on SVB

/*#define CLKINP_QT       20.0
#define CLKINP_SVB      19.2*/

//PLL0: Main PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		2GHz
POSTDIV output: 	1GHz
HSDIV0 output: 		500MHz
HSDIV1 output: 		250MHz
HSDIV2 output: 		200MHz
HSDIV3 output: 		133.33MHz
HSDIV4 output: 		80MHz
HSDIV5 output: 		50MHz
HSDIV6 output: 		250MHz
HSDIV7 output: 		200MHz
HSDIV8 output: 		333.33MHz
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL0_FBDIV             80	//104   //fbdiv
#define MAIN_PLL0_FRACDIV           0	//2796203    //fracdiv
#define MAIN_PLL0_PREDIV            1     //prediv
#define MAIN_PLL0_POSTDIV1          2     //postdiv1
#define MAIN_PLL0_POSTDIV2          1     //postdiv2
#define MAIN_PLL0_HSDIV0_DIV_VAL    3     //4
#define MAIN_PLL0_HSDIV1_DIV_VAL    7	//8
#define MAIN_PLL0_HSDIV2_DIV_VAL    9	//10
#define MAIN_PLL0_HSDIV3_DIV_VAL    14	//15
#define MAIN_PLL0_HSDIV4_DIV_VAL    24	//25
#define MAIN_PLL0_HSDIV5_DIV_VAL    19	//20
#define MAIN_PLL0_HSDIV6_DIV_VAL    3	//4
#define MAIN_PLL0_HSDIV7_DIV_VAL    4	//5
#define MAIN_PLL0_HSDIV8_DIV_VAL    2	//3
#define MAIN_PLL0_SSMOD_SPREAD      0x1F  //spread
#define MAIN_PLL0_SSMOD_MODDIV      -1    //mod_div
#define MAIN_PLL0_SSMOD_DOWNSPREAD  1     //downspread

//Main PLL0 PLL Controller Parameters
#define MAIN_CTRL_BPDIV  	0 //AUXCLK=BPCLK=REFCLK for controller
#define MAIN_CTRL_OD1 		0 //OBSCLK=REFCLK for controller
#define MAIN_CTRL_DIV1 		1 //500MHZ SYSCLK1 from HSDIV_CLKOUT1


//PLL1:  Peripheral 0 PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		1920MHz
POSTDIV output: 	960MHz
HSDIV0 output: 		192MHz
HSDIV1 output: 		320MHz
HSDIV2 output: 		192MHz
HSDIV3 output: 		192MHz
HSDIV4 output: 		N/A
HSDIV5 output: 		192MHz
HSDIV6 output: 		19.2MHz
HSDIV7 output: 		24MHz
HSDIV8 output: 		20MHz
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL1_FBDIV             76//100
#define MAIN_PLL1_FRACDIV           13421773//-1
#define MAIN_PLL1_PREDIV            1
#define MAIN_PLL1_POSTDIV1          2
#define MAIN_PLL1_POSTDIV2          1
#define MAIN_PLL1_HSDIV0_DIV_VAL    9	//10
#define MAIN_PLL1_HSDIV1_DIV_VAL    5	//6
#define MAIN_PLL1_HSDIV2_DIV_VAL    9	//10
#define MAIN_PLL1_HSDIV3_DIV_VAL    9	//10
#define MAIN_PLL1_HSDIV4_DIV_VAL    -1 //No HSDIV
#define MAIN_PLL1_HSDIV5_DIV_VAL    4	//5
#define MAIN_PLL1_HSDIV6_DIV_VAL    49	//50
#define MAIN_PLL1_HSDIV7_DIV_VAL    39	//40
#define MAIN_PLL1_HSDIV8_DIV_VAL    47	//48
#define MAIN_PLL1_SSMOD_SPREAD      0x1F
#define MAIN_PLL1_SSMOD_MODDIV      -1
#define MAIN_PLL1_SSMOD_DOWNSPREAD  1


//PLL2: Peripheral 1 PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		1800MHz
POSTDIV output: 	900MHz
HSDIV0 output: 		360MHz
HSDIV1 output: 		600MHz
HSDIV2 output: 		200MHz
HSDIV3 output: 		300MHz
HSDIV4 output: 		100MHz
HSDIV5 output: 		450MHz
HSDIV6 output: 		225MHz
HSDIV7 output: 		120MHz
HSDIV8 output:  	N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL2_FBDIV             72//93
#define MAIN_PLL2_FRACDIV           0//12582912
#define MAIN_PLL2_PREDIV            1
#define MAIN_PLL2_POSTDIV1          2
#define MAIN_PLL2_POSTDIV2          1
#define MAIN_PLL2_HSDIV0_DIV_VAL    4	//5
#define MAIN_PLL2_HSDIV1_DIV_VAL    2	//3
#define MAIN_PLL2_HSDIV2_DIV_VAL    8	//9
#define MAIN_PLL2_HSDIV3_DIV_VAL    5	//6
#define MAIN_PLL2_HSDIV4_DIV_VAL    17	//18
#define MAIN_PLL2_HSDIV5_DIV_VAL    1	//2
#define MAIN_PLL2_HSDIV6_DIV_VAL    3	//4
#define MAIN_PLL2_HSDIV7_DIV_VAL    14 	//15
#define MAIN_PLL2_HSDIV8_DIV_VAL    -1 	//No HSDIV
#define MAIN_PLL2_SSMOD_SPREAD      0x1F
#define MAIN_PLL2_SSMOD_MODDIV      -1
#define MAIN_PLL2_SSMOD_DOWNSPREAD  1


//PLL3: CPSW9x PLL
/* Frequencies:

PLL input:		19.2MHz
VCO output:		2500MHz
POSTDIV output: 	2500MHz
HSDIV0 output: 		250MHz
HSDIV1 output: 		250MHz
HSDIV2 output: 		200MHz (approx 208.3 MHz)
HSDIV3 output: 		250MHz
HSDIV4 output: 		156.25MHz
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL3_FBDIV             80//130
#define MAIN_PLL3_FRACDIV           0//3495253
#define MAIN_PLL3_PREDIV            1
#define MAIN_PLL3_POSTDIV1          1
#define MAIN_PLL3_POSTDIV2          1
#define MAIN_PLL3_HSDIV0_DIV_VAL    9 //10
#define MAIN_PLL3_HSDIV1_DIV_VAL    9 //10
#define MAIN_PLL3_HSDIV2_DIV_VAL    11 //12
#define MAIN_PLL3_HSDIV3_DIV_VAL    9 //10
#define MAIN_PLL3_HSDIV4_DIV_VAL    15 //16
#define MAIN_PLL3_HSDIV5_DIV_VAL    -1
#define MAIN_PLL3_HSDIV6_DIV_VAL    -1
#define MAIN_PLL3_HSDIV7_DIV_VAL    -1
#define MAIN_PLL3_HSDIV8_DIV_VAL    -1
#define MAIN_PLL3_SSMOD_SPREAD      0x1F
#define MAIN_PLL3_SSMOD_MODDIV      -1
#define MAIN_PLL3_SSMOD_DOWNSPREAD  1


//PLL4: Audio PLL 0
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		1180MHz
POSTDIV output: 	1180MHz
HSDIV0 output: 		196.67MHz
HSDIV1 output: 		295MHz
HSDIV2 output: 		196.67MHz
HSDIV3 output: 		12.292MHz
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output:  	N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL4_FBDIV             47//61
#define MAIN_PLL4_FRACDIV           3119220//7689557
#define MAIN_PLL4_PREDIV            1
#define MAIN_PLL4_POSTDIV1          1
#define MAIN_PLL4_POSTDIV2          1
#define MAIN_PLL4_HSDIV0_DIV_VAL    5 //6
#define MAIN_PLL4_HSDIV1_DIV_VAL    3 //4
#define MAIN_PLL4_HSDIV2_DIV_VAL    5 //6
#define MAIN_PLL4_HSDIV3_DIV_VAL    95 //96
#define MAIN_PLL4_HSDIV4_DIV_VAL    -1
#define MAIN_PLL4_HSDIV5_DIV_VAL    -1
#define MAIN_PLL4_HSDIV6_DIV_VAL    -1
#define MAIN_PLL4_HSDIV7_DIV_VAL    -1
#define MAIN_PLL4_HSDIV8_DIV_VAL    -1
#define MAIN_PLL4_SSMOD_SPREAD      0x1F
#define MAIN_PLL4_SSMOD_MODDIV      -1
#define MAIN_PLL4_SSMOD_DOWNSPREAD  1


//PLL5: Video PLL
/* Frequencies:

PLL input:		19.2MHz
VCO output:		2750MHz
POSTDIV output: 	2750MHz
HSDIV0 output:		687.5MHz (approx)
HSDIV1 output: 		550MHz (approx)
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output:  	N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL5_FBDIV             110//143
#define MAIN_PLL5_FRACDIV           0//3844779
#define MAIN_PLL5_PREDIV            1
#define MAIN_PLL5_POSTDIV1          1
#define MAIN_PLL5_POSTDIV2          1
#define MAIN_PLL5_HSDIV0_DIV_VAL    3 //4
#define MAIN_PLL5_HSDIV1_DIV_VAL    4 //5
#define MAIN_PLL5_HSDIV2_DIV_VAL    -1
#define MAIN_PLL5_HSDIV3_DIV_VAL    -1
#define MAIN_PLL5_HSDIV4_DIV_VAL    -1
#define MAIN_PLL5_HSDIV5_DIV_VAL    -1
#define MAIN_PLL5_HSDIV6_DIV_VAL    -1
#define MAIN_PLL5_HSDIV7_DIV_VAL    -1
#define MAIN_PLL5_HSDIV8_DIV_VAL    -1
#define MAIN_PLL5_SSMOD_SPREAD      0x1F
#define MAIN_PLL5_SSMOD_MODDIV      -1
#define MAIN_PLL5_SSMOD_DOWNSPREAD  1


//PLL6: GPU PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		750MHz
HSDIV1 output: 		N/A
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output:  	N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL6_FBDIV             60//156
#define MAIN_PLL6_FRACDIV           0//4194304
#define MAIN_PLL6_PREDIV            1
#define MAIN_PLL6_POSTDIV1          1
#define MAIN_PLL6_POSTDIV2          1
#define MAIN_PLL6_HSDIV0_DIV_VAL    3 //4
#define MAIN_PLL6_HSDIV1_DIV_VAL    -1
#define MAIN_PLL6_HSDIV2_DIV_VAL    -1
#define MAIN_PLL6_HSDIV3_DIV_VAL    -1
#define MAIN_PLL6_HSDIV4_DIV_VAL    -1
#define MAIN_PLL6_HSDIV5_DIV_VAL    -1
#define MAIN_PLL6_HSDIV6_DIV_VAL    -1
#define MAIN_PLL6_HSDIV7_DIV_VAL    -1
#define MAIN_PLL6_HSDIV8_DIV_VAL    -1
#define MAIN_PLL6_SSMOD_SPREAD      0x1F
#define MAIN_PLL6_SSMOD_MODDIV      -1
#define MAIN_PLL6_SSMOD_DOWNSPREAD  1


//PLL7: C7x PLL
/* Frequencies:

PLL input:		19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		1GHz
HSDIV1 output: 		N/A
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL7_FBDIV             80//156
#define MAIN_PLL7_FRACDIV           0//4194304
#define MAIN_PLL7_PREDIV            1
#define MAIN_PLL7_POSTDIV1          1
#define MAIN_PLL7_POSTDIV2          1
#define MAIN_PLL7_HSDIV0_DIV_VAL    2 //3
#define MAIN_PLL7_HSDIV1_DIV_VAL    -1
#define MAIN_PLL7_HSDIV2_DIV_VAL    -1
#define MAIN_PLL7_HSDIV3_DIV_VAL    -1
#define MAIN_PLL7_HSDIV4_DIV_VAL    -1
#define MAIN_PLL7_HSDIV5_DIV_VAL    -1
#define MAIN_PLL7_HSDIV6_DIV_VAL    -1
#define MAIN_PLL7_HSDIV7_DIV_VAL    -1
#define MAIN_PLL7_HSDIV8_DIV_VAL    -1
#define MAIN_PLL7_SSMOD_SPREAD      0x1F
#define MAIN_PLL7_SSMOD_MODDIV      -1
#define MAIN_PLL7_SSMOD_DOWNSPREAD  1


//PLL8: ARM0 PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		2000MHz
POSTDIV output: 	2000MHz
HSDIV0 output: 		2GHz
HSDIV1 output: 		N/A
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL8_FBDIV             80//104
#define MAIN_PLL8_FRACDIV           0//2796203
#define MAIN_PLL8_PREDIV            1
#define MAIN_PLL8_POSTDIV1          1
#define MAIN_PLL8_POSTDIV2          1
#define MAIN_PLL8_HSDIV0_DIV_VAL    0 //1
#define MAIN_PLL8_HSDIV1_DIV_VAL    -1
#define MAIN_PLL8_HSDIV2_DIV_VAL    -1
#define MAIN_PLL8_HSDIV3_DIV_VAL    -1
#define MAIN_PLL8_HSDIV4_DIV_VAL    -1
#define MAIN_PLL8_HSDIV5_DIV_VAL    -1
#define MAIN_PLL8_HSDIV6_DIV_VAL    -1
#define MAIN_PLL8_HSDIV7_DIV_VAL    -1
#define MAIN_PLL8_HSDIV8_DIV_VAL    -1
#define MAIN_PLL8_SSMOD_SPREAD      0x1F
#define MAIN_PLL8_SSMOD_MODDIV      -1
#define MAIN_PLL8_SSMOD_DOWNSPREAD  1


//PLL9: Not Present **************************************
//PLL10: Not Present *************************************
//PLL11: Not Present *************************************


//PLL12: DDR FracF PLL
/* Frequencies:

PLL input:		19.2MHz
VCO output:		2130MHz
POSTDIV output: 	2130MHz
HSDIV0 output: 		1065MHz
HSDIV1 output: 		N/A
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL12_FBDIV            74//111
#define MAIN_PLL12_FRACDIV          10737419//1572864
#define MAIN_PLL12_PREDIV           1
#define MAIN_PLL12_POSTDIV1         1
#define MAIN_PLL12_POSTDIV2         1
#define MAIN_PLL12_HSDIV0_DIV_VAL   1 //2
#define MAIN_PLL12_HSDIV1_DIV_VAL   -1
#define MAIN_PLL12_HSDIV2_DIV_VAL   -1
#define MAIN_PLL12_HSDIV3_DIV_VAL   -1
#define MAIN_PLL12_HSDIV4_DIV_VAL   -1
#define MAIN_PLL12_HSDIV5_DIV_VAL   -1
#define MAIN_PLL12_HSDIV6_DIV_VAL   -1
#define MAIN_PLL12_HSDIV7_DIV_VAL   -1
#define MAIN_PLL12_HSDIV8_DIV_VAL   -1
#define MAIN_PLL12_SSMOD_SPREAD     -1
#define MAIN_PLL12_SSMOD_MODDIV     -1
#define MAIN_PLL12_SSMOD_DOWNSPREAD -1

//#define MAIN_PLL12_FBDIV_DDR_1866    97
//#define MAIN_PLL12_FRACDIV_DDR_1866  3145728
#define MAIN_PLL12_FBDIV_DDR_1866    74
#define MAIN_PLL12_FRACDIV_DDR_1866  10737419

#define MAIN_PLL12_FBDIV_DDR_1600    83
#define MAIN_PLL12_FRACDIV_DDR_1600  5592405

#define MAIN_PLL12_FBDIV_DDR_3200    166
#define MAIN_PLL12_FRACDIV_DDR_3200  11184810

#define MAIN_PLL12_FBDIV_DDR_2664    138
#define MAIN_PLL12_FRACDIV_DDR_2664  12582912


//PLL13: C66 PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		2.7GHz
POSTDIV output: 	2.7GHz
HSDIV0 output: 		1.35GHz
HSDIV1 output: 		1.35GHz
HSDIV2 output: 		67.5MHz (not connected to anything)
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL13_FBDIV            108//140
#define MAIN_PLL13_FRACDIV          0//10485760
#define MAIN_PLL13_PREDIV           1
#define MAIN_PLL13_POSTDIV1         1
#define MAIN_PLL13_POSTDIV2         1
#define MAIN_PLL13_HSDIV0_DIV_VAL   1 //2
#define MAIN_PLL13_HSDIV1_DIV_VAL   1 //2
#define MAIN_PLL13_HSDIV2_DIV_VAL   39 //40
#define MAIN_PLL13_HSDIV3_DIV_VAL   -1
#define MAIN_PLL13_HSDIV4_DIV_VAL   -1
#define MAIN_PLL13_HSDIV5_DIV_VAL   -1
#define MAIN_PLL13_HSDIV6_DIV_VAL   -1
#define MAIN_PLL13_HSDIV7_DIV_VAL   -1
#define MAIN_PLL13_HSDIV8_DIV_VAL   -1
#define MAIN_PLL13_SSMOD_SPREAD     0x1F
#define MAIN_PLL13_SSMOD_MODDIV     -1
#define MAIN_PLL13_SSMOD_DOWNSPREAD 1


//PLL14: Main Pulsar PLL
/* Frequencies:

PLL input:  	19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		1GHz
HSDIV1 output: 		1GHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL14_FBDIV            80//156
#define MAIN_PLL14_FRACDIV          0//4194304
#define MAIN_PLL14_PREDIV           1
#define MAIN_PLL14_POSTDIV1         1
#define MAIN_PLL14_POSTDIV2         1
#define MAIN_PLL14_HSDIV0_DIV_VAL   2 //3
#define MAIN_PLL14_HSDIV1_DIV_VAL   2 //3
#define MAIN_PLL14_HSDIV2_DIV_VAL   -1
#define MAIN_PLL14_HSDIV3_DIV_VAL   -1
#define MAIN_PLL14_HSDIV4_DIV_VAL   -1
#define MAIN_PLL14_HSDIV5_DIV_VAL   -1
#define MAIN_PLL14_HSDIV6_DIV_VAL   -1
#define MAIN_PLL14_HSDIV7_DIV_VAL   -1
#define MAIN_PLL14_HSDIV8_DIV_VAL   -1
#define MAIN_PLL14_SSMOD_SPREAD     0x1F
#define MAIN_PLL14_SSMOD_MODDIV     -1
#define MAIN_PLL14_SSMOD_DOWNSPREAD 1


//PLL15: Audio PLL 1
/* Frequencies:

PLL input:		19.2MHz
VCO output:		1180MHz
POSTDIV output: 	1180MHz
HSDIV0 output: 		196.7MHz
HSDIV1 output: 		295MHz
HSDIV2 output: 		196.7MHz
HSDIV3 output: 		12.29MHz
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL15_FBDIV            43//61
#define MAIN_PLL15_FRACDIV          5906654//7689557
#define MAIN_PLL15_PREDIV           1
#define MAIN_PLL15_POSTDIV1         1
#define MAIN_PLL15_POSTDIV2         1
#define MAIN_PLL15_HSDIV0_DIV_VAL   5 //6
#define MAIN_PLL15_HSDIV1_DIV_VAL   3 //4
#define MAIN_PLL15_HSDIV2_DIV_VAL   5 //6
#define MAIN_PLL15_HSDIV3_DIV_VAL   95 //96
#define MAIN_PLL15_HSDIV4_DIV_VAL   -1
#define MAIN_PLL15_HSDIV5_DIV_VAL   -1
#define MAIN_PLL15_HSDIV6_DIV_VAL   -1
#define MAIN_PLL15_HSDIV7_DIV_VAL   -1
#define MAIN_PLL15_HSDIV8_DIV_VAL   -1
#define MAIN_PLL15_SSMOD_SPREAD     0x1F
#define MAIN_PLL15_SSMOD_MODDIV     -1
#define MAIN_PLL15_SSMOD_DOWNSPREAD 1


//PLL16: DSS PLL0
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		600MHz
HSDIV1 output: 		600MHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL16_FBDIV            48//156
#define MAIN_PLL16_FRACDIV          0//4194304
#define MAIN_PLL16_PREDIV           1
#define MAIN_PLL16_POSTDIV1         1
#define MAIN_PLL16_POSTDIV2         1
#define MAIN_PLL16_HSDIV0_DIV_VAL   4 //5
#define MAIN_PLL16_HSDIV1_DIV_VAL   4 //5
#define MAIN_PLL16_HSDIV2_DIV_VAL   -1
#define MAIN_PLL16_HSDIV3_DIV_VAL   -1
#define MAIN_PLL16_HSDIV4_DIV_VAL   -1
#define MAIN_PLL16_HSDIV5_DIV_VAL   -1
#define MAIN_PLL16_HSDIV6_DIV_VAL   -1
#define MAIN_PLL16_HSDIV7_DIV_VAL   -1
#define MAIN_PLL16_HSDIV8_DIV_VAL   -1
#define MAIN_PLL16_SSMOD_SPREAD     0x1F
#define MAIN_PLL16_SSMOD_MODDIV     -1
#define MAIN_PLL16_SSMOD_DOWNSPREAD 1

#define MAIN_PLL16_FBDIV_DSS_2970 		154
#define MAIN_PLL16_FRACDIV_DSS_2970 	11534336

#define MAIN_PLL16_FBDIV_DSS_2345 		122
#define MAIN_PLL16_FRACDIV_DSS_2345 	3058347

#define MAIN_PLL16_FBDIV_DSS_2898 		150
#define MAIN_PLL16_FRACDIV_DSS_2898 	15728640

#define MAIN_PLL16_FBDIV_DSS_2613 		136
#define MAIN_PLL16_FRACDIV_DSS_2613 	1634031

#define MAIN_PLL16_FBDIV_DSS_2133 		111
#define MAIN_PLL16_FRACDIV_DSS_2133		1572864


//PLL17: DSS PLL1
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		600MHz
HSDIV1 output: 		600MHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL17_FBDIV            48//156
#define MAIN_PLL17_FRACDIV          0//4194304
#define MAIN_PLL17_PREDIV           1
#define MAIN_PLL17_POSTDIV1         1
#define MAIN_PLL17_POSTDIV2         1
#define MAIN_PLL17_HSDIV0_DIV_VAL   4 //5
#define MAIN_PLL17_HSDIV1_DIV_VAL   4 //5
#define MAIN_PLL17_HSDIV2_DIV_VAL   -1
#define MAIN_PLL17_HSDIV3_DIV_VAL   -1
#define MAIN_PLL17_HSDIV4_DIV_VAL   -1
#define MAIN_PLL17_HSDIV5_DIV_VAL   -1
#define MAIN_PLL17_HSDIV6_DIV_VAL   -1
#define MAIN_PLL17_HSDIV7_DIV_VAL   -1
#define MAIN_PLL17_HSDIV8_DIV_VAL   -1
#define MAIN_PLL17_SSMOD_SPREAD     0x1F
#define MAIN_PLL17_SSMOD_MODDIV     -1
#define MAIN_PLL17_SSMOD_DOWNSPREAD 1

#define MAIN_PLL17_FBDIV_DSS_2970 		154
#define MAIN_PLL17_FRACDIV_DSS_2970 	11534336

#define MAIN_PLL17_FBDIV_DSS_2345 		122
#define MAIN_PLL17_FRACDIV_DSS_2345 	3058347

#define MAIN_PLL17_FBDIV_DSS_2898 		150
#define MAIN_PLL17_FRACDIV_DSS_2898 	15728640

#define MAIN_PLL17_FBDIV_DSS_2613 		136
#define MAIN_PLL17_FRACDIV_DSS_2613 	1634031

#define MAIN_PLL17_FBDIV_DSS_2133 		111
#define MAIN_PLL17_FRACDIV_DSS_2133		1572864


//PLL18: DSS PLL2
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		600MHz
HSDIV1 output: 		600MHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL18_FBDIV            48//156
#define MAIN_PLL18_FRACDIV          0//4194304
#define MAIN_PLL18_PREDIV           1
#define MAIN_PLL18_POSTDIV1         1
#define MAIN_PLL18_POSTDIV2         1
#define MAIN_PLL18_HSDIV0_DIV_VAL   4 //5
#define MAIN_PLL18_HSDIV1_DIV_VAL   4 //5
#define MAIN_PLL18_HSDIV2_DIV_VAL   -1
#define MAIN_PLL18_HSDIV3_DIV_VAL   -1
#define MAIN_PLL18_HSDIV4_DIV_VAL   -1
#define MAIN_PLL18_HSDIV5_DIV_VAL   -1
#define MAIN_PLL18_HSDIV6_DIV_VAL   -1
#define MAIN_PLL18_HSDIV7_DIV_VAL   -1
#define MAIN_PLL18_HSDIV8_DIV_VAL   -1
#define MAIN_PLL18_SSMOD_SPREAD     0x1F
#define MAIN_PLL18_SSMOD_MODDIV     -1
#define MAIN_PLL18_SSMOD_DOWNSPREAD 1

#define MAIN_PLL18_FBDIV_DSS_2970 		154
#define MAIN_PLL18_FRACDIV_DSS_2970 	11534336

#define MAIN_PLL18_FBDIV_DSS_2345 		122
#define MAIN_PLL18_FRACDIV_DSS_2345 	3058347

#define MAIN_PLL18_FBDIV_DSS_2898 		150
#define MAIN_PLL18_FRACDIV_DSS_2898 	15728640

#define MAIN_PLL18_FBDIV_DSS_2000       104
#define MAIN_PLL18_FRACDIV_DSS_2000     2796203

#define MAIN_PLL18_FBDIV_DSS_2500       130
#define MAIN_PLL18_FRACDIV_DSS_2500     3495253

#define MAIN_PLL18_FBDIV_DSS_3000       156
#define MAIN_PLL18_FRACDIV_DSS_3000     4194304

//PLL19: DSS PLL3
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		600MHz
HSDIV1 output: 		600MHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL19_FBDIV            48//156
#define MAIN_PLL19_FRACDIV          0//4194304
#define MAIN_PLL19_PREDIV           1
#define MAIN_PLL19_POSTDIV1         1
#define MAIN_PLL19_POSTDIV2         1
#define MAIN_PLL19_HSDIV0_DIV_VAL   4 //5
#define MAIN_PLL19_HSDIV1_DIV_VAL   4 //5
#define MAIN_PLL19_HSDIV2_DIV_VAL   -1
#define MAIN_PLL19_HSDIV3_DIV_VAL   -1
#define MAIN_PLL19_HSDIV4_DIV_VAL   -1
#define MAIN_PLL19_HSDIV5_DIV_VAL   -1
#define MAIN_PLL19_HSDIV6_DIV_VAL   -1
#define MAIN_PLL19_HSDIV7_DIV_VAL   -1
#define MAIN_PLL19_HSDIV8_DIV_VAL   -1
#define MAIN_PLL19_SSMOD_SPREAD     0x1F
#define MAIN_PLL19_SSMOD_MODDIV     -1
#define MAIN_PLL19_SSMOD_DOWNSPREAD 1

#define MAIN_PLL19_FBDIV_DSS_2970 		154
#define MAIN_PLL19_FRACDIV_DSS_2970 	11534336


//PLL20: Not Present *************************************
//PLL21: Not Present *************************************
//PLL22: Not Present *************************************


//PLL23: DSS PLL7
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		3GHz
POSTDIV output: 	3GHz
HSDIV0 output: 		600MHz
HSDIV1 output: 		600MHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread:

 */
#define MAIN_PLL23_FBDIV            48//156
#define MAIN_PLL23_FRACDIV          0//4194304
#define MAIN_PLL23_PREDIV           1
#define MAIN_PLL23_POSTDIV1         1
#define MAIN_PLL23_POSTDIV2         1
#define MAIN_PLL23_HSDIV0_DIV_VAL   4 //5
#define MAIN_PLL23_HSDIV1_DIV_VAL   4 //5
#define MAIN_PLL23_HSDIV2_DIV_VAL   -1
#define MAIN_PLL23_HSDIV3_DIV_VAL   -1
#define MAIN_PLL23_HSDIV4_DIV_VAL   -1
#define MAIN_PLL23_HSDIV5_DIV_VAL   -1
#define MAIN_PLL23_HSDIV6_DIV_VAL   -1
#define MAIN_PLL23_HSDIV7_DIV_VAL   -1
#define MAIN_PLL23_HSDIV8_DIV_VAL   -1
#define MAIN_PLL23_SSMOD_SPREAD     0x1F
#define MAIN_PLL23_SSMOD_MODDIV     -1
#define MAIN_PLL23_SSMOD_DOWNSPREAD 1

#define MAIN_PLL23_FBDIV_DSS_2970 		154
#define MAIN_PLL23_FRACDIV_DSS_2970 	11534336


//PLL25: Image Processing PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		2880MHz
POSTDIV output: 	2880MHz
HSDIV0 output: 		480MHz
HSDIV1 output: 		720MHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MAIN_PLL25_FBDIV            104//150
#define MAIN_PLL25_FRACDIV          0//-1
#define MAIN_PLL25_PREDIV           1
#define MAIN_PLL25_POSTDIV1         1
#define MAIN_PLL25_POSTDIV2         1
#define MAIN_PLL25_HSDIV0_DIV_VAL   5 //6
#define MAIN_PLL25_HSDIV1_DIV_VAL   3 //4
#define MAIN_PLL25_HSDIV2_DIV_VAL   -1
#define MAIN_PLL25_HSDIV3_DIV_VAL   -1
#define MAIN_PLL25_HSDIV4_DIV_VAL   -1
#define MAIN_PLL25_HSDIV5_DIV_VAL   -1
#define MAIN_PLL25_HSDIV6_DIV_VAL   -1
#define MAIN_PLL25_HSDIV7_DIV_VAL   -1
#define MAIN_PLL25_HSDIV8_DIV_VAL   -1
#define MAIN_PLL25_SSMOD_SPREAD     0x1F
#define MAIN_PLL25_SSMOD_MODDIV     -1
#define MAIN_PLL25_SSMOD_DOWNSPREAD 1

#define MAIN_PLL25_FBDIV_DMPAC_520 	            135	// 2600MHz VCO output
#define MAIN_PLL25_FRACDIV_DMPAC_520            6990507 // 2600MHz VCO output
#define MAIN_PLL25_HSDIV0_DMPAC_520_DIV_VAL 	4 //5 (this is going to DMPAC)
#define MAIN_PLL25_HSDIV1_DMPAC_520_DIV_VAL 	3 //4 (this is going to VPAC)


//MCU PLL0: MCU PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		2000MHz
POSTDIV output: 	2GHz
HSDIV0 output: 		1GHz
HSDIV1 output: 		60.6MHz
HSDIV2 output: 		N/A
HSDIV3 output: 		N/A
HSDIV4 output: 		N/A
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MCU_PLL0_FBDIV              80//104
#define MCU_PLL0_FRACDIV            0//2796203
#define MCU_PLL0_PREDIV             1
#define MCU_PLL0_POSTDIV1           1
#define MCU_PLL0_POSTDIV2           1
#define MCU_PLL0_HSDIV0_DIV_VAL     1 //2
#define MCU_PLL0_HSDIV1_DIV_VAL     32 //33
#define MCU_PLL0_HSDIV2_DIV_VAL     -1
#define MCU_PLL0_HSDIV3_DIV_VAL     -1
#define MCU_PLL0_HSDIV4_DIV_VAL     -1
#define MCU_PLL0_HSDIV5_DIV_VAL     -1
#define MCU_PLL0_HSDIV6_DIV_VAL     -1
#define MCU_PLL0_HSDIV7_DIV_VAL     -1
#define MCU_PLL0_HSDIV8_DIV_VAL     -1
#define MCU_PLL0_SSMOD_SPREAD       0x1F
#define MCU_PLL0_SSMOD_MODDIV       -1
#define MCU_PLL0_SSMOD_DOWNSPREAD   1

//MCU0 PLL PLL Controller Parameters
#define MCU_CTRL_BPDIV  0 //AUXCLK=BPCLK=REFCLK for controller
#define MCU_CTRL_OD1    0 //OBSCLK=REFCLK for controller
#define MCU_CTRL_DIV1   1 //1000MHZ SYSCLK1 from MCU PLL CLKOUT


//MCU PLL1: MCU Pulsar PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		2400MHz
POSTDIV output: 	2400MHz
HSDIV0 output: 		400MHz
HSDIV1 output: 		60MHz
HSDIV2 output: 		80MHz
HSDIV3 output: 		96MHz
HSDIV4 output: 		400MHz
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MCU_PLL1_FBDIV              96//125
#define MCU_PLL1_FRACDIV            0//-1
#define MCU_PLL1_PREDIV             1
#define MCU_PLL1_POSTDIV1           1
#define MCU_PLL1_POSTDIV2           1
#define MCU_PLL1_HSDIV0_DIV_VAL     5 //6
#define MCU_PLL1_HSDIV1_DIV_VAL     39 //40
#define MCU_PLL1_HSDIV2_DIV_VAL     29 //30
#define MCU_PLL1_HSDIV3_DIV_VAL     24 //25
#define MCU_PLL1_HSDIV4_DIV_VAL     5 //6
#define MCU_PLL1_HSDIV5_DIV_VAL     -1
#define MCU_PLL1_HSDIV6_DIV_VAL     -1
#define MCU_PLL1_HSDIV7_DIV_VAL     -1
#define MCU_PLL1_HSDIV8_DIV_VAL     -1
#define MCU_PLL1_SSMOD_SPREAD       0x1F
#define MCU_PLL1_SSMOD_MODDIV       -1
#define MCU_PLL1_SSMOD_DOWNSPREAD   1


//MCU PLL2: MCU CPSW PLL
/* Frequencies:

PLL input: 		19.2MHz
VCO output:		2000MHz
POSTDIV output: 	2000MHz
HSDIV0 output: 		250MHz
HSDIV1 output: 		200MHz
HSDIV2 output: 		200MHz
HSDIV3 output: 		80MHz
HSDIV4 output: 		333.33MHz
HSDIV5 output: 		N/A
HSDIV6 output: 		N/A
HSDIV7 output: 		N/A
HSDIV8 output: 		N/A
MODSS configuration: 
- Spread: 3.125%
- Modulator Divider: %-by-1
- Downspread or centerspread: downspread

 */
#define MCU_PLL2_FBDIV              80//104
#define MCU_PLL2_FRACDIV            0//2796203
#define MCU_PLL2_PREDIV             1
#define MCU_PLL2_POSTDIV1           1
#define MCU_PLL2_POSTDIV2           1
#define MCU_PLL2_HSDIV0_DIV_VAL     7 //8
#define MCU_PLL2_HSDIV1_DIV_VAL     9 //10
#define MCU_PLL2_HSDIV2_DIV_VAL     9 //10
#define MCU_PLL2_HSDIV3_DIV_VAL     24 //25
#define MCU_PLL2_HSDIV4_DIV_VAL     5 //6
#define MCU_PLL2_HSDIV5_DIV_VAL     -1
#define MCU_PLL2_HSDIV6_DIV_VAL     -1
#define MCU_PLL2_HSDIV7_DIV_VAL     -1
#define MCU_PLL2_HSDIV8_DIV_VAL     -1
#define MCU_PLL2_SSMOD_SPREAD       0x1F
#define MCU_PLL2_SSMOD_MODDIV       -1
#define MCU_PLL2_SSMOD_DOWNSPREAD   1

/* =========================================================================
 * PLL register offsets (from J721E_PLL_MMR.gel)
 * ========================================================================= */
#define PLL_PID                         (0x00U)
#define PLL_CONFIG                      (0x08U)
#define CONTROL                         (0x20U)
#define STATUS                          (0x24U)
#define FREQ_CONTROL_0                  (0x30U)
#define FREQ_CONTROL_1                  (0x34U)
#define OUTPUT_DIV_CONTROL              (0x38U)
#define SSMOD_CONTROL                   (0x40U)
#define SSMOD_SPREAD                    (0x44U)
#define CAL_CONTROL                     (0x60U)
#define CAL_STATUS                      (0x64U)

/* =========================================================================
 * PLL index map (from J721E_PLL_MMR.gel)
 * ========================================================================= */
#define MAIN_PLL_INDEX                  0
#define PER0_PLL_INDEX                  1
#define PER1_PLL_INDEX                  2
#define CPSW9_PLL_INDEX                 3
#define AUDIO0_PLL_INDEX                4
#define VIDEO_PLL_INDEX                 5
#define GPU_PLL_INDEX                   6
#define C7X_PLL_INDEX                   7
#define ARM0_PLL_INDEX                  8
#define DDR_PLL_INDEX                   12
#define C66_PLL_INDEX                   13
#define MAIN_R5F_PLL_INDEX              14
#define AUDIO1_PLL_INDEX                15
#define DSS0_PLL_INDEX                  16
#define DSS1_PLL_INDEX                  17
#define DSS2_PLL_INDEX                  18
#define DSS3_PLL_INDEX                  19
#define DSS7_PLL_INDEX                  23
#define MLB_PLL_INDEX                   24
#define VISION_PLL_INDEX                25
#define MCU_R5F_PLL_INDEX               0
#define MCU_DOM_PLL_INDEX               1
#define MCU_CPSW_PLL_INDEX              2

/* =========================================================================
 * Special programming indices (from J721E_PLL_MMR.gel)
 * ========================================================================= */
#define OFC1                            0
#define ARM_250MHZ                      1
#define ARM_500MHZ                      2
#define ARM_1GHZ                        3
#define ARM_2GHZ                        4
#define VPAC_720                        5
#define DMPAC_520                       6
#define DDR_BYPASS                      7
#define DDR_400                         8
#define DDR_800                         9
#define DDR_1066                        10
#define DDR_1600                        11
#define DDR_1866                        12
#define DDR_2133                        13
#define DDR_3200                        14
#define DSS_2970                        15
#define DSS_2345                        16
#define DSS_2898                        17
#define DSS_2613                        18
#define DSS_2133                        19
#define DSS_2000                        20
#define DSS_2500                        21
#define DSS_3000                        22
#define DDR_2664                        23

/* =========================================================================
 * PLL type IDs (from J721E_PLL_OFC1.gel)
 * ========================================================================= */
#define FRAC_PLL                        0
#define FRAC_F_PLL                      1
#define DESKEW_PLL                      2

/* =========================================================================
 * PLL unlock/lock values (from J721E_PLL_OFC1.gel)
 * ========================================================================= */
#define KICK0_UNLOCK                    0x68EF3490U
#define KICK1_UNLOCK                    0xD172BC5AU
#define KICK_LOCK                       0x00000000U

/* =========================================================================
 * PLLCTRL base addresses (from J721E_PLL_OFC1.gel)
 * ========================================================================= */
#define CSL_PLLCTRL0_BASE               (0x00410000U)
#define CSL_WKUP_PLLCTRL0_BASE          (0x42010000U)

/* =========================================================================
 * PLLCTRL register offsets (from J721E_PLL_MMR.gel)
 * ========================================================================= */
#define PLL_CTRL_PLLCTL_OFFSET          0x0100
#define PLL_CTRL_OCSEL_OFFSET           0x0104
#define PLL_CTRL_PREDIV_OFFSET          0x0114
#define PLL_CTRL_PLLDIV1_OFFSET         0x0118
#define PLL_CTRL_PLLDIV2_OFFSET         0x011C
#define PLL_CTRL_PLLDIV3_OFFSET         0x0120
#define PLL_CTRL_OSCDIV1_OFFSET         0x0124
#define PLL_CTRL_POSTDIV_OFFSET         0x0128
#define PLL_CTRL_BPDIV_OFFSET           0x012C
#define PLL_CTRL_PLLCMD_OFFSET          0x0138
#define PLL_CTRL_PLLSTAT_OFFSET         0x013C
#define PLL_CTRL_ALNCTL_OFFSET          0x0140
#define PLL_CTRL_CKEN_OFFSET            0x0148
#define PLL_CTRL_CKSTAT_OFFSET          0x014C
#define PLL_CTRL_SYSTAT_OFFSET          0x0150

/* =========================================================================
 * MAIN_PLL24 divider values (MLB PLL -- unused in OFC1, stubbed out)
 * ========================================================================= */
#define MAIN_PLL24_FBDIV                0
#define MAIN_PLL24_FRACDIV              0
#define MAIN_PLL24_PREDIV               1
#define MAIN_PLL24_POSTDIV1             1
#define MAIN_PLL24_POSTDIV2             1
#define MAIN_PLL24_HSDIV0_DIV_VAL       -1
#define MAIN_PLL24_HSDIV1_DIV_VAL       -1
#define MAIN_PLL24_HSDIV2_DIV_VAL       -1
#define MAIN_PLL24_HSDIV3_DIV_VAL       -1
#define MAIN_PLL24_HSDIV4_DIV_VAL       -1
#define MAIN_PLL24_HSDIV5_DIV_VAL       -1
#define MAIN_PLL24_HSDIV6_DIV_VAL       -1
#define MAIN_PLL24_HSDIV7_DIV_VAL       -1
#define MAIN_PLL24_HSDIV8_DIV_VAL       -1
#define MAIN_PLL24_SSMOD_SPREAD         -1
#define MAIN_PLL24_SSMOD_MODDIV         -1
#define MAIN_PLL24_SSMOD_DOWNSPREAD     -1

/* =========================================================================
 * Helpers
 * ========================================================================= */
#define REG32(addr)                      (*(volatile uint32_t *)(uintptr_t)(addr))
#define BIT(x)                           (1U << (x))

/* =========================================================================
 * PSC helpers — direct physical addresses, no RAT translation
 * ========================================================================= */

static int set_main_psc_state(uint32_t pd_id, uint32_t md_id, uint32_t pd_state, uint32_t md_state)
{
    volatile uint32_t *mdctl =
        (volatile uint32_t *)(uintptr_t)(MAIN_PSC_BASE + PSC_MDCTL00 + (4U * md_id));
    volatile uint32_t *mdstat =
        (volatile uint32_t *)(uintptr_t)(MAIN_PSC_BASE + PSC_MDSTAT00 + (4U * md_id));
    volatile uint32_t *pdctl =
        (volatile uint32_t *)(uintptr_t)(MAIN_PSC_BASE + PSC_PDCTL00 + (4U * pd_id));
    volatile uint32_t *pdstat =
        (volatile uint32_t *)(uintptr_t)(MAIN_PSC_BASE + PSC_PDSTAT00 + (4U * pd_id));

    if (((*pdstat & 0x1U) == pd_state) && ((*mdstat & 0x1fU) == md_state))
        return 1;

    uint32_t loop_cnt = 0;
    while ((loop_cnt < PSC_TIMEOUT) && ((REG32(MAIN_PSC_BASE + PSC_PTSTAT) & BIT(pd_id)) != 0))
        loop_cnt++;
    if (loop_cnt >= PSC_TIMEOUT)
        return 0;

    *pdctl = (*pdctl & ~0x1U) | pd_state;
    *mdctl = (*mdctl & ~0x1fU) | md_state;

    REG32(MAIN_PSC_BASE + PSC_PTCMD) |= BIT(pd_id);

    loop_cnt = 0;
    while ((loop_cnt < PSC_TIMEOUT) && ((REG32(MAIN_PSC_BASE + PSC_PTSTAT) & BIT(pd_id)) != 0))
        loop_cnt++;
    if (loop_cnt >= PSC_TIMEOUT)
        return 0;

    return ((*pdstat & 0x1U) == pd_state) && ((*mdstat & 0x1fU) == md_state);
}

/******************************************************************************
 *
 * NAME:
 *      Set_WKUP_PSC_State
 *
 * PURPOSE:
 *      Set a new power state for the specified power domain pd_id and module
 *		domain md_id. Wait for the power transition to complete.
 *
 * USAGE:
 *      This routine can be called as:
 *
 *      Set_WKUP_PSC_State(unsigned int pd_id, unsigned int md_id, unsigned int pd_state, unsigned
 * int md_state)
 *
 *      pd_id - (i) power domain id
 *
 *      md_id - (i) module domain id
 *
 *		pd_state - (i) new power domain state value to set
 *					0 = PSC_PD_OFF
 *					1 = PSC_PD_ON
 *
 *      md_state - (i) new module domain state value to set
 *                  0 = PSC_SYNCRESETDISABLE (IP reset asserted and clock gated)
 *                  1 = PSC_SYNCRESET (IP reset asserted and clock running)
 *                  2 = PSC_DISABLE (IP reset released and clock gated, MMRs retained)
 *                  3 = PSC_ENABLE (IP reset released and clock running)
 *
 * RETURN VALUE:
 *      0 if ok, !=0 for error
 *
 * REFERENCE:
 *
 *****************************************************************************/
static int Set_WKUP_PSC_State(unsigned int pd_id,
                       unsigned int md_id,
                       unsigned int pd_state,
                       unsigned int md_state)
{
    volatile uint32_t *mdctl =
        (volatile uint32_t *)(uintptr_t)(WAKEUP_PSC_BASE + PSC_MDCTL00 + (4U * md_id));
    volatile uint32_t *mdstat =
        (volatile uint32_t *)(uintptr_t)(WAKEUP_PSC_BASE + PSC_MDSTAT00 + (4U * md_id));
    volatile uint32_t *pdctl =
        (volatile uint32_t *)(uintptr_t)(WAKEUP_PSC_BASE + PSC_PDCTL00 + (4U * pd_id));
    volatile uint32_t *pdstat =
        (volatile uint32_t *)(uintptr_t)(WAKEUP_PSC_BASE + PSC_PDSTAT00 + (4U * pd_id));

    if (((*pdstat & 0x1U) == pd_state) && ((*mdstat & 0x1fU) == md_state))
        return 1;

    uint32_t loop_cnt = 0;
    while ((loop_cnt < PSC_TIMEOUT) && ((REG32(WAKEUP_PSC_BASE + PSC_PTSTAT) & BIT(pd_id)) != 0))
        loop_cnt++;
    if (loop_cnt >= PSC_TIMEOUT)
        return 0;

    *pdctl = (*pdctl & ~0x1U) | pd_state;
    *mdctl = (*mdctl & ~0x1fU) | md_state;

    REG32(WAKEUP_PSC_BASE + PSC_PTCMD) |= BIT(pd_id);

    loop_cnt = 0;
    while ((loop_cnt < PSC_TIMEOUT) && ((REG32(WAKEUP_PSC_BASE + PSC_PTSTAT) & BIT(pd_id)) != 0))
        loop_cnt++;
    if (loop_cnt >= PSC_TIMEOUT)
        return 0;

    return ((*pdstat & 0x1U) == pd_state) && ((*mdstat & 0x1fU) == md_state);
}

//Get WKUP PSC STATE
static int Get_WKUP_PSC_State( unsigned int pd_id, unsigned int md_id )
{
    volatile uint32_t *mdstat =
        (volatile uint32_t *)(uintptr_t)(WAKEUP_PSC_BASE + PSC_MDSTAT00 + (4U * md_id));
    volatile uint32_t *pdstat =
        (volatile uint32_t *)(uintptr_t)(WAKEUP_PSC_BASE + PSC_PDSTAT00 + (4U * pd_id));

    uint32_t pd_state = *pdstat & 0x1U;
    uint32_t md_state = *mdstat & 0x1fU;

    if (pd_state == PSC_PD_OFF)
        printf_("Power Domain: Off");
    else if (pd_state == PSC_PD_ON)
        printf_("Power Domain: On");
    else
        printf_("Power Domain: ERR");

    if (md_state == PSC_SYNCRESETDISABLE)
        printf_("Module State: SyncResetDiable");
    else if (md_state == PSC_SYNCRESET)
        printf_("Module State: SyncReset");
    else if (md_state == PSC_DISABLE)
        printf_("Module State: Disable");
    else if (md_state == PSC_ENABLE)
        printf_("Module State: Enable");
    else
        printf_("Module State: Error");

    return 1;
}

/*
 * Note: the following 2 functions are re-used from the J7 AVV MMR library.
 * Those files can be found in ${K3_AVV_REPO}/framework/mmr/src/j7es_mmr.c
 * and ${K3_AVV_REPO}/framework/mmr/include/j7es_mmr.h.
 */
/**
 * \brief   Write the unlocking keys to the locking registers.
 *
 * \param   kick0 		 		The first lock register.
 *
 * \param   kick1 				The second lock register.
 *
 * \return  status 				This should return 0 on a successful unlock.
 */
static int MMR_Unlock_One(uint32_t * kick0, uint32_t * kick1)
{
    // initialize the status variable
    uint32_t status = 1;

    // if either of the kick lock registers are locked
    if (!(*kick0 & 0x1) | !(*kick1 & 0x1)){
        // unlock the partition by writing the unlock values to the kick lock registers
        *kick0 = KICK0_UNLOCK;
        *kick1 = KICK1_UNLOCK;
    }

    // check to see if either of the kick registers are unlocked.
    if (!(*kick0 & 0x1)){
        status = 0;
    }

    // return the status to the calling program
    return status;

}

/* =========================================================================
 * MMR/Register access primitives
 * ========================================================================= */

/**
 * \brief   Read a 32-bit value from a memory-mapped register.
 */
static uint32_t Read_MMR(uint32_t address)
{
    return REG32(address);
}

/**
 * \brief   Write a 32-bit value to a memory-mapped register.
 */
static void Write_MMR(uint32_t address, uint32_t value)
{
    REG32(address) = value;
}

/**
 * \brief   Write a value into a bit field within a memory-mapped register.
 *
 * \param   address   The register address.
 * \param   value     The value to write into the field.
 * \param   width     The number of bits in the field.
 * \param   shift     The bit position of the field LSB.
 */
static void Write_MMR_Field(uint32_t address, uint32_t value, uint32_t width, uint32_t shift)
{
    uint32_t mask = ((1UL << (width)) - 1UL) << (shift);
    uint32_t reg_val = REG32(address);
    reg_val &= ~mask;
    reg_val |= ((value) << (shift)) & mask;
    REG32(address) = reg_val;
}

/**
 * \brief   Read a bit field from a memory-mapped register.
 *
 * \param   address   The register address.
 * \param   width     The number of bits in the field.
 * \param   shift     The bit position of the field LSB.
 *
 * \return  The value of the bit field, extracted and right-aligned.
 */
static uint32_t Read_MMR_Field(uint32_t address, uint32_t width, uint32_t shift)
{
    uint32_t mask = (1UL << (width)) - 1UL;
    return (REG32(address) >> (shift)) & mask;
}

/* =========================================================================
 * PLL programming helpers
 * ========================================================================= */

/**
 * \brief   Program a high-speed divider (HSDIV).
 *
 * Translates Program_HSDIV() from J721E_PLL_OFC1.gel:929-966.
 * Runs on MCU1_0 with direct physical addresses -- no RAT offset needed.
 *
 * \param   Base_Address    The base address of the PLL MMR instance.
 * \param   address_offset  Ignored (retained for parameter compatibility; always 0).
 * \param   PLL_index       The PLL's index (0-25).
 * \param   HSDIV_index     The HSDIV index within the PLL (0-8).
 * \param   hsdiv_value     The raw divider value (hardware applies +1).
 */
static void Program_HSDIV(uint32_t Base_Address, uint32_t address_offset,
                          uint32_t PLL_index, uint32_t HSDIV_index,
                          uint32_t hsdiv_value)
{
    uint32_t hsdiv_reg = Base_Address + (PLL_index * 0x1000) + (HSDIV_index * 4U + 0x80U);

    /* Assert reset (bit 31) */
    Write_MMR_Field(hsdiv_reg, 1U, 1, 31);

    /* Set the divider value in the bottom 7 bits */
    Write_MMR_Field(hsdiv_reg, hsdiv_value, 7, 0);

    /* Enable clock output (bit 15) if not already set */
    if (!Read_MMR_Field(hsdiv_reg, 1, 15))
    {
        Write_MMR_Field(hsdiv_reg, 1U, 1, 15);
    }

    /* De-assert reset (bit 31) */
    Write_MMR_Field(hsdiv_reg, 0U, 1, 31);
}

/**
 * \brief   Program a PLL Controller (PLLCTRL).
 *
 * Translates Program_PLLCTRL() from J721E_PLL_OFC1.gel:1058-1195.
 * Runs on MCU1_0 with direct physical addresses -- no RAT offset needed.
 *
 * \param   Base_Address    Base address of the PLL Controller (CSL_PLLCTRL0_BASE
 *                          or CSL_WKUP_PLLCTRL0_BASE).
 * \param   address_offset  Ignored (retained for parameter compatibility; always 0).
 */
static void Program_PLLCTRL(uint32_t Base_Address, uint32_t address_offset)
{
    uint32_t obsclk_div1 = 0;
    uint32_t bp_divider = 0;
    uint32_t output_div1 = 0;

    /* Grab PLL Controller parameter values */
    if (Base_Address == CSL_PLLCTRL0_BASE)
    {
        obsclk_div1 = MAIN_CTRL_OD1;
        bp_divider  = MAIN_CTRL_BPDIV;
        output_div1 = MAIN_CTRL_DIV1;
    }
    else if (Base_Address == CSL_WKUP_PLLCTRL0_BASE)
    {
        obsclk_div1 = MCU_CTRL_OD1;
        bp_divider  = MCU_CTRL_BPDIV;
        output_div1 = MCU_CTRL_DIV1;
    }

    /* Clear BIT 0 (PLLCTL enable) */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLCTL_OFFSET), 0U, 1, 0);

    /* Clear BIT 5 (bypass mode on) */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLCTL_OFFSET), 0U, 1, 5);

    /* Set BPDIV */
    Write_MMR_Field((Base_Address + PLL_CTRL_BPDIV_OFFSET), bp_divider, 8, 0);

    /* Set OBS divider - enable clock (bit 15) and divide value */
    Write_MMR((Base_Address + PLL_CTRL_OSCDIV1_OFFSET), (obsclk_div1 | (1U << 15)));

    /* Clear GOSET */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLCMD_OFFSET), 0U, 1, 0);

    /* Poll GOSTAT to clear */
    while ((Read_MMR((Base_Address + PLL_CTRL_PLLSTAT_OFFSET)) & 0x1U)) { }

    /* Set PLLDIV1 */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLDIV1_OFFSET), output_div1, 8, 0);

    /* Clear ALN1 on ALNCTL */
    Write_MMR_Field((Base_Address + PLL_CTRL_ALNCTL_OFFSET), 0U, 1, 0);

    /* Set OBSCLK observation clock input -- OCSEL = 0x12 (point C) */
    Write_MMR((Base_Address + 0x104U), 0x12U);

    /* Set clock control: bit 0 (AUXCLK) cleared, bit 1 (OBSCLK) set */
    Write_MMR((Base_Address + 0x148U), 0x2U);

    /* Set GOSET to 1 */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLCMD_OFFSET), 1U, 1, 0);

    /* Poll on GOSTAT to be 0 */
    while ((Read_MMR((Base_Address + PLL_CTRL_PLLSTAT_OFFSET)) & 0x1U)) { }

    /* Enable PLL Controller (write to bit 0 in control register) */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLCTL_OFFSET), 1U, 1, 0);

    /* Clear GOSET to 0 */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLCMD_OFFSET), 0U, 1, 0);

    /* Clear PLLRST (bit 3) to release reset */
    Write_MMR_Field((Base_Address + PLL_CTRL_PLLCTL_OFFSET), 0U, 1, 3);
}

/**
 * \brief   Unlock the partition for a specific PLL.
 *
 * \param   Base_Address 		The base address of the PLL MMR instance.
 *
 * \param   PLL_index 			The index of the PLL (one PLL per partition).
 *
 * \return  none
 */
void Unlock_PLL_MMR(uint32_t Base_Address, uint32_t address_offset, uint32_t PLL_index){
    uint32_t first_mmr, second_mmr;
    //Calculate the first lock register address based on the PLL index.
    first_mmr = 0x10 + (PLL_index * 0x1000) + Base_Address + address_offset;
    //Calculate tthe second lock register address based on the PLL index.
    second_mmr = 0x14 + (PLL_index * 0x1000) + Base_Address + address_offset;
    //Unlock the MMR region with those addresses.
    MMR_Unlock_One((uint32_t *)(first_mmr),(uint32_t *)(second_mmr));
}

/**
 * \brief   Program the PLL, any associated HSDIVs, PLL Controllers, and SSMODs.
 *
 * \param   Base_Address		The base address of the PLL registers. This
 *                       		changes depending on which domain the PLL is in.
 *
 * \param   address_offset 		Address offset for M3 RAT compatibility.
 *
 * \param   PLL_index    		The PLL's index, ranges from 0-25. These are
 *                   			defined in J7_PLL_MMR.GEL.
 *
 * \param   Clocking_Scheme     The operating frequency condition specified.
 *                         		Changing this will change the OFC of the chip.
 *
 * \return  none
 */
void Program_PLL(uint32_t Base_Address, uint32_t address_offset, uint32_t PLL_index, uint32_t Clocking_Scheme){
    //For debugging only
    int debug_info = 1;
    //All other variables
    uint32_t fout = 0;
    uint32_t fref = 0;
    uint32_t frefdiv = 0;
    uint32_t fbdiv = 0;
    signed int frac = 0;
    uint32_t refdiv = 0;
    uint32_t postdiv1 = 0;
    uint32_t postdiv2 = 0;
    signed int div_val_0 = 0;
    signed int div_val_1 = 0;
    signed int div_val_2 = 0;
    signed int div_val_3 = 0;
    signed int div_val_4 = 0;
    signed int div_val_5 = 0;
    signed int div_val_6 = 0;
    signed int div_val_7 = 0;
    signed int div_val_8 = 0;
    uint32_t ssmod_spreadval = 0;
    uint32_t ssmod_input_div = 0;
    uint32_t ssmod_spreadtype = 0;
    unsigned int PLL_config_info;
    unsigned int HSDIV_Presence = 0;
    unsigned int Num_HSDIV = 0;
    unsigned int hsdiv_value = 0;
    unsigned int SSM_Type = 0;
    unsigned int SSM_Wavetable_Presence = 0;
    unsigned int PLL_Type = 0;
    unsigned int Is_PLL_Locked;
    unsigned int i = 0; //used for counting for any for loops.
    unsigned int temp = 0; //used for mathematical calculations.

    //Unlock the PLL
    Unlock_PLL_MMR(Base_Address, address_offset, PLL_index);

    if(debug_info){
        printf_("Unlocked PLL MMRs.\n");
    }

    //Grab the PLL configuration from memory.
    PLL_config_info = Read_MMR(Base_Address + address_offset + (PLL_index * 0x1000) + PLL_CONFIG);

    if(debug_info){
        printf_("Read configuration MMRs.\n");
    }

    //Extract the PLL configuration from the register value.
    HSDIV_Presence = (PLL_config_info & 0xFFFF0000) >> 16; 	//Mask & shift the HSDIV presence field.
    SSM_Type = (PLL_config_info & 0x00001800) >> 11; 	//Mask & shift the ssm_type field.
    SSM_Wavetable_Presence = (PLL_config_info & 0x00000100 >> 8); //Mask & shift the ssm_wvtbl field.
    PLL_Type = (PLL_config_info & 0x00000003); 			//Extract the pll_type field.

    //Need to reform Num_HSDIV. The index is programmed where each bit position represents the existence of a high speed divider.
    temp = HSDIV_Presence; //use a temp variable to avoid having to re-read the MMR to restore the value.
    if(debug_info){
        printf_("temp value (HSDIV_Presence) = %x\n", temp);
        printf_("HSDIV presence value = %x\n", HSDIV_Presence);
    }

    while(temp != 0){
        temp = temp >> 1;	//shift right by 1.
        Num_HSDIV++; //Count how many bit positions are set to "1".
    }

    if(debug_info){
        printf_("Number of hsidvs: %d\n",Num_HSDIV);
        printf_("Parsed PLL configuration information.\n");
    }

    //*************************************************************************
    //Gather the PLL divider values for the speific PLL. Special programming modes will be provided later.
    if(PLL_index == 0 && (Base_Address == CSL_PLL0_CFG_BASE)){
        fbdiv = MAIN_PLL0_FBDIV;
        frac = MAIN_PLL0_FRACDIV;
        frefdiv = MAIN_PLL0_PREDIV;
        postdiv1 = MAIN_PLL0_POSTDIV1;
        postdiv2 = MAIN_PLL0_POSTDIV2;
        div_val_0 = MAIN_PLL0_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL0_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL0_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL0_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL0_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL0_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL0_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL0_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL0_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL0_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL0_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL0_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 1 && (Base_Address == CSL_PLL0_CFG_BASE)){
        fbdiv = MAIN_PLL1_FBDIV;
        frac = MAIN_PLL1_FRACDIV;
        frefdiv = MAIN_PLL1_PREDIV;
        postdiv1 = MAIN_PLL1_POSTDIV1;
        postdiv2 = MAIN_PLL1_POSTDIV2;
        div_val_0 = MAIN_PLL1_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL1_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL1_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL1_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL1_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL1_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL1_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL1_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL1_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL1_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL1_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL1_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 2 && (Base_Address == CSL_PLL0_CFG_BASE)){
        fbdiv = MAIN_PLL2_FBDIV;
        frac = MAIN_PLL2_FRACDIV;
        frefdiv = MAIN_PLL2_PREDIV;
        postdiv1 = MAIN_PLL2_POSTDIV1;
        postdiv2 = MAIN_PLL2_POSTDIV2;
        div_val_0 = MAIN_PLL2_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL2_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL2_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL2_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL2_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL2_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL2_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL2_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL2_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL2_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL2_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL2_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 3){
        fbdiv = MAIN_PLL3_FBDIV;
        frac = MAIN_PLL3_FRACDIV;
        frefdiv = MAIN_PLL3_PREDIV;
        postdiv1 = MAIN_PLL3_POSTDIV1;
        postdiv2 = MAIN_PLL3_POSTDIV2;
        div_val_0 = MAIN_PLL3_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL3_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL3_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL3_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL3_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL3_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL3_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL3_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL3_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL3_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL3_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL3_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 4){
        fbdiv = MAIN_PLL4_FBDIV;
        frac = MAIN_PLL4_FRACDIV;
        frefdiv = MAIN_PLL4_PREDIV;
        postdiv1 = MAIN_PLL4_POSTDIV1;
        postdiv2 = MAIN_PLL4_POSTDIV2;
        div_val_0 = MAIN_PLL4_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL4_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL4_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL4_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL4_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL4_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL4_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL4_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL4_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL4_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL4_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL4_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 5){
        fbdiv = MAIN_PLL5_FBDIV;
        frac = MAIN_PLL5_FRACDIV;
        frefdiv = MAIN_PLL5_PREDIV;
        postdiv1 = MAIN_PLL5_POSTDIV1;
        postdiv2 = MAIN_PLL5_POSTDIV2;
        div_val_0 = MAIN_PLL5_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL5_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL5_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL5_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL5_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL5_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL5_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL5_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL5_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL5_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL5_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL5_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 6){
        fbdiv = MAIN_PLL6_FBDIV;
        frac = MAIN_PLL6_FRACDIV;
        frefdiv = MAIN_PLL6_PREDIV;
        postdiv1 = MAIN_PLL6_POSTDIV1;
        postdiv2 = MAIN_PLL6_POSTDIV2;
        div_val_0 = MAIN_PLL6_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL6_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL6_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL6_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL6_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL6_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL6_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL6_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL6_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL6_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL6_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL6_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 7){
        fbdiv = MAIN_PLL7_FBDIV;
        frac = MAIN_PLL7_FRACDIV;
        frefdiv = MAIN_PLL7_PREDIV;
        postdiv1 = MAIN_PLL7_POSTDIV1;
        postdiv2 = MAIN_PLL7_POSTDIV2;
        div_val_0 = MAIN_PLL7_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL7_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL7_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL7_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL7_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL7_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL7_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL7_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL7_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL7_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL7_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL7_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 8){
        fbdiv = MAIN_PLL8_FBDIV;
        frac = MAIN_PLL8_FRACDIV;
        frefdiv = MAIN_PLL8_PREDIV;
        postdiv1 = MAIN_PLL8_POSTDIV1;
        postdiv2 = MAIN_PLL8_POSTDIV2;
        div_val_0 = MAIN_PLL8_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL8_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL8_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL8_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL8_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL8_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL8_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL8_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL8_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL8_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL8_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL8_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 12){
        fbdiv = MAIN_PLL12_FBDIV;
        frac = MAIN_PLL12_FRACDIV;
        frefdiv = MAIN_PLL12_PREDIV;
        postdiv1 = MAIN_PLL12_POSTDIV1;
        postdiv2 = MAIN_PLL12_POSTDIV2;
        div_val_0 = MAIN_PLL12_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL12_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL12_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL12_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL12_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL12_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL12_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL12_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL12_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL12_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL12_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL12_SSMOD_DOWNSPREAD;
        //Clocking schemes for different DDR OFCs
        if(Clocking_Scheme == OFC1){
            fbdiv = MAIN_PLL12_FBDIV;
            frac = MAIN_PLL12_FRACDIV;
        }else if(Clocking_Scheme == DDR_1600){
            fbdiv = MAIN_PLL12_FBDIV_DDR_1600;
            frac = MAIN_PLL12_FRACDIV_DDR_1600;
        }else if(Clocking_Scheme == DDR_1866){
            fbdiv = MAIN_PLL12_FBDIV_DDR_1866;
            frac = MAIN_PLL12_FRACDIV_DDR_1866;
        }else if(Clocking_Scheme == DDR_2664){
            fbdiv = MAIN_PLL12_FBDIV_DDR_2664;
            frac = MAIN_PLL12_FRACDIV_DDR_2664;
        }else if(Clocking_Scheme == DDR_3200){
            fbdiv = MAIN_PLL12_FBDIV_DDR_3200;
            frac = MAIN_PLL12_FRACDIV_DDR_3200;
        }
    }else 	if(PLL_index == 13){
        fbdiv = MAIN_PLL13_FBDIV;
        frac = MAIN_PLL13_FRACDIV;
        frefdiv = MAIN_PLL13_PREDIV;
        postdiv1 = MAIN_PLL13_POSTDIV1;
        postdiv2 = MAIN_PLL13_POSTDIV2;
        div_val_0 = MAIN_PLL13_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL13_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL13_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL13_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL13_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL13_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL13_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL13_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL13_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL13_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL13_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL13_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 14){
        fbdiv = MAIN_PLL14_FBDIV;
        frac = MAIN_PLL14_FRACDIV;
        frefdiv = MAIN_PLL14_PREDIV;
        postdiv1 = MAIN_PLL14_POSTDIV1;
        postdiv2 = MAIN_PLL14_POSTDIV2;
        div_val_0 = MAIN_PLL14_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL14_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL14_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL14_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL14_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL14_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL14_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL14_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL14_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL14_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL14_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL14_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 15){
        fbdiv = MAIN_PLL15_FBDIV;
        frac = MAIN_PLL15_FRACDIV;
        frefdiv = MAIN_PLL15_PREDIV;
        postdiv1 = MAIN_PLL15_POSTDIV1;
        postdiv2 = MAIN_PLL15_POSTDIV2;
        div_val_0 = MAIN_PLL15_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL15_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL15_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL15_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL15_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL15_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL15_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL15_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL15_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL15_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL15_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL15_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 16){
        fbdiv = MAIN_PLL16_FBDIV;
        frac = MAIN_PLL16_FRACDIV;
        frefdiv = MAIN_PLL16_PREDIV;
        postdiv1 = MAIN_PLL16_POSTDIV1;
        postdiv2 = MAIN_PLL16_POSTDIV2;
        div_val_0 = MAIN_PLL16_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL16_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL16_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL16_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL16_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL16_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL16_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL16_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL16_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL16_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL16_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL16_SSMOD_DOWNSPREAD;
        if(Clocking_Scheme == DSS_2970){
            fbdiv = MAIN_PLL16_FBDIV_DSS_2970;
            frac = MAIN_PLL16_FRACDIV_DSS_2970;
        }else if(Clocking_Scheme == DSS_2345){
            fbdiv = MAIN_PLL16_FBDIV_DSS_2345;
            frac = MAIN_PLL16_FRACDIV_DSS_2345;
        }else if(Clocking_Scheme == DSS_2898){
            fbdiv = MAIN_PLL16_FBDIV_DSS_2898;
            frac = MAIN_PLL16_FRACDIV_DSS_2898;
        }else if(Clocking_Scheme == DSS_2613){
            fbdiv = MAIN_PLL16_FBDIV_DSS_2613;
            frac = MAIN_PLL16_FRACDIV_DSS_2613;
        }else if(Clocking_Scheme == DSS_2133){
            fbdiv = MAIN_PLL16_FBDIV_DSS_2133;
            frac = MAIN_PLL16_FRACDIV_DSS_2133;
        }
    }else 	if(PLL_index == 17){
        fbdiv = MAIN_PLL17_FBDIV;
        frac = MAIN_PLL17_FRACDIV;
        frefdiv = MAIN_PLL17_PREDIV;
        postdiv1 = MAIN_PLL17_POSTDIV1;
        postdiv2 = MAIN_PLL17_POSTDIV2;
        div_val_0 = MAIN_PLL17_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL17_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL17_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL17_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL17_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL17_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL17_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL17_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL17_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL17_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL17_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL17_SSMOD_DOWNSPREAD;
        if(Clocking_Scheme == DSS_2970){
            fbdiv = MAIN_PLL17_FBDIV_DSS_2970;
            frac = MAIN_PLL17_FRACDIV_DSS_2970;
        }else if(Clocking_Scheme == DSS_2345){
            fbdiv = MAIN_PLL17_FBDIV_DSS_2345;
            frac = MAIN_PLL17_FRACDIV_DSS_2345;
        }else if(Clocking_Scheme == DSS_2898){
            fbdiv = MAIN_PLL17_FBDIV_DSS_2898;
            frac = MAIN_PLL17_FRACDIV_DSS_2898;
        }else if(Clocking_Scheme == DSS_2613){
            fbdiv = MAIN_PLL17_FBDIV_DSS_2613;
            frac = MAIN_PLL17_FRACDIV_DSS_2613;
        }else if(Clocking_Scheme == DSS_2133){
            fbdiv = MAIN_PLL17_FBDIV_DSS_2133;
            frac = MAIN_PLL17_FRACDIV_DSS_2133;
        }
    }else 	if(PLL_index == 18){
        fbdiv = MAIN_PLL18_FBDIV;
        frac = MAIN_PLL18_FRACDIV;
        frefdiv = MAIN_PLL18_PREDIV;
        postdiv1 = MAIN_PLL18_POSTDIV1;
        postdiv2 = MAIN_PLL18_POSTDIV2;
        div_val_0 = MAIN_PLL18_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL18_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL18_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL18_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL18_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL18_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL18_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL18_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL18_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL18_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL18_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL18_SSMOD_DOWNSPREAD;
        if(Clocking_Scheme == DSS_2970){
            fbdiv = MAIN_PLL18_FBDIV_DSS_2970;
            frac = MAIN_PLL18_FRACDIV_DSS_2970;
        }else if(Clocking_Scheme == DSS_2345){
            fbdiv = MAIN_PLL18_FBDIV_DSS_2345;
            frac = MAIN_PLL18_FRACDIV_DSS_2345;
        }else if(Clocking_Scheme == DSS_2898){
            fbdiv = MAIN_PLL18_FBDIV_DSS_2898;
            frac = MAIN_PLL18_FRACDIV_DSS_2898;
        }else if(Clocking_Scheme == DSS_2000){
            fbdiv = MAIN_PLL18_FBDIV_DSS_2000;
            frac = MAIN_PLL18_FRACDIV_DSS_2000;
        }else if(Clocking_Scheme == DSS_2500){
            fbdiv = MAIN_PLL18_FBDIV_DSS_2500;
            frac = MAIN_PLL18_FRACDIV_DSS_2500;
        }else if(Clocking_Scheme == DSS_3000){
            fbdiv = MAIN_PLL18_FBDIV_DSS_3000;
            frac = MAIN_PLL18_FRACDIV_DSS_3000;
        }
    }else 	if(PLL_index == 19){
        fbdiv = MAIN_PLL19_FBDIV;
        frac = MAIN_PLL19_FRACDIV;
        frefdiv = MAIN_PLL19_PREDIV;
        postdiv1 = MAIN_PLL19_POSTDIV1;
        postdiv2 = MAIN_PLL19_POSTDIV2;
        div_val_0 = MAIN_PLL19_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL19_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL19_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL19_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL19_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL19_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL19_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL19_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL19_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL19_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL19_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL19_SSMOD_DOWNSPREAD;
        if(Clocking_Scheme == DSS_2970){
            fbdiv = MAIN_PLL19_FBDIV_DSS_2970;
            frac = MAIN_PLL19_FRACDIV_DSS_2970;
        }
    }else 	if(PLL_index == 23){
        fbdiv = MAIN_PLL23_FBDIV;
        frac = MAIN_PLL23_FRACDIV;
        frefdiv = MAIN_PLL23_PREDIV;
        postdiv1 = MAIN_PLL23_POSTDIV1;
        postdiv2 = MAIN_PLL23_POSTDIV2;
        div_val_0 = MAIN_PLL23_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL23_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL23_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL23_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL23_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL23_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL23_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL23_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL23_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL23_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL23_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL23_SSMOD_DOWNSPREAD;
        if(Clocking_Scheme == DSS_2970){
            fbdiv = MAIN_PLL23_FBDIV_DSS_2970;
            frac = MAIN_PLL23_FRACDIV_DSS_2970;
        }
    }else 	if(PLL_index == 24){
        fbdiv = MAIN_PLL24_FBDIV;
        frac = MAIN_PLL24_FRACDIV;
        frefdiv = MAIN_PLL24_PREDIV;
        postdiv1 = MAIN_PLL24_POSTDIV1;
        postdiv2 = MAIN_PLL24_POSTDIV2;
        div_val_0 = MAIN_PLL24_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL24_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL24_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL24_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL24_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL24_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL24_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL24_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL24_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL24_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL24_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL24_SSMOD_DOWNSPREAD;
    }else 	if(PLL_index == 25){
        fbdiv = MAIN_PLL25_FBDIV;
        frac = MAIN_PLL25_FRACDIV;
        frefdiv = MAIN_PLL25_PREDIV;
        postdiv1 = MAIN_PLL25_POSTDIV1;
        postdiv2 = MAIN_PLL25_POSTDIV2;
        div_val_0 = MAIN_PLL25_HSDIV0_DIV_VAL;
        div_val_1 = MAIN_PLL25_HSDIV1_DIV_VAL;
        div_val_2 = MAIN_PLL25_HSDIV2_DIV_VAL;
        div_val_3 = MAIN_PLL25_HSDIV3_DIV_VAL;
        div_val_4 = MAIN_PLL25_HSDIV4_DIV_VAL;
        div_val_5 = MAIN_PLL25_HSDIV5_DIV_VAL;
        div_val_6 = MAIN_PLL25_HSDIV6_DIV_VAL;
        div_val_7 = MAIN_PLL25_HSDIV7_DIV_VAL;
        div_val_8 = MAIN_PLL25_HSDIV8_DIV_VAL;
        ssmod_spreadval = MAIN_PLL25_SSMOD_SPREAD;
        ssmod_input_div = MAIN_PLL25_SSMOD_MODDIV;
        ssmod_spreadtype = MAIN_PLL25_SSMOD_DOWNSPREAD;
        //Other clocking schemes will start here.
        //Special Clocking schemes.
        if(Clocking_Scheme == DMPAC_520){
            //Set the Feedback divider and the HSDIV divider values.
            fbdiv = MAIN_PLL25_FBDIV_DMPAC_520;
            frac = MAIN_PLL25_FRACDIV_DMPAC_520;
            div_val_0 = MAIN_PLL25_HSDIV0_DMPAC_520_DIV_VAL;
            div_val_1 = MAIN_PLL25_HSDIV1_DMPAC_520_DIV_VAL;
        }
    }else 	if((PLL_index == 0) && (Base_Address == (CSL_MCU_PLL0_CFG_BASE))){
        fbdiv = MCU_PLL0_FBDIV;
        frac = MCU_PLL0_FRACDIV;
        frefdiv = MCU_PLL0_PREDIV;
        postdiv1 = MCU_PLL0_POSTDIV1;
        postdiv2 = MCU_PLL0_POSTDIV2;
        div_val_0 = MCU_PLL0_HSDIV0_DIV_VAL;
        div_val_1 = MCU_PLL0_HSDIV1_DIV_VAL;
        div_val_2 = MCU_PLL0_HSDIV2_DIV_VAL;
        div_val_3 = MCU_PLL0_HSDIV3_DIV_VAL;
        div_val_4 = MCU_PLL0_HSDIV4_DIV_VAL;
        div_val_5 = MCU_PLL0_HSDIV5_DIV_VAL;
        div_val_6 = MCU_PLL0_HSDIV6_DIV_VAL;
        div_val_7 = MCU_PLL0_HSDIV7_DIV_VAL;
        div_val_8 = MCU_PLL0_HSDIV8_DIV_VAL;
        ssmod_spreadval = MCU_PLL0_SSMOD_SPREAD;
        ssmod_input_div = MCU_PLL0_SSMOD_MODDIV;
        ssmod_spreadtype = MCU_PLL0_SSMOD_DOWNSPREAD;
    }else 	if((PLL_index == 1) && (Base_Address == (CSL_MCU_PLL0_CFG_BASE))){
        fbdiv = MCU_PLL1_FBDIV;
        frac = MCU_PLL1_FRACDIV;
        frefdiv = MCU_PLL1_PREDIV;
        postdiv1 = MCU_PLL1_POSTDIV1;
        postdiv2 = MCU_PLL1_POSTDIV2;
        div_val_0 = MCU_PLL1_HSDIV0_DIV_VAL;
        div_val_1 = MCU_PLL1_HSDIV1_DIV_VAL;
        div_val_2 = MCU_PLL1_HSDIV2_DIV_VAL;
        div_val_3 = MCU_PLL1_HSDIV3_DIV_VAL;
        div_val_4 = MCU_PLL1_HSDIV4_DIV_VAL;
        div_val_5 = MCU_PLL1_HSDIV5_DIV_VAL;
        div_val_6 = MCU_PLL1_HSDIV6_DIV_VAL;
        div_val_7 = MCU_PLL1_HSDIV7_DIV_VAL;
        div_val_8 = MCU_PLL1_HSDIV8_DIV_VAL;
        ssmod_spreadval = MCU_PLL1_SSMOD_SPREAD;
        ssmod_input_div = MCU_PLL1_SSMOD_MODDIV;
        ssmod_spreadtype = MCU_PLL1_SSMOD_DOWNSPREAD;
    }else 	if((PLL_index == 2) && (Base_Address == (CSL_MCU_PLL0_CFG_BASE))){
        fbdiv = MCU_PLL2_FBDIV;
        frac = MCU_PLL2_FRACDIV;
        frefdiv = MCU_PLL2_PREDIV;
        postdiv1 = MCU_PLL2_POSTDIV1;
        postdiv2 = MCU_PLL2_POSTDIV2;
        div_val_0 = MCU_PLL2_HSDIV0_DIV_VAL;
        div_val_1 = MCU_PLL2_HSDIV1_DIV_VAL;
        div_val_2 = MCU_PLL2_HSDIV2_DIV_VAL;
        div_val_3 = MCU_PLL2_HSDIV3_DIV_VAL;
        div_val_4 = MCU_PLL2_HSDIV4_DIV_VAL;
        div_val_5 = MCU_PLL2_HSDIV5_DIV_VAL;
        div_val_6 = MCU_PLL2_HSDIV6_DIV_VAL;
        div_val_7 = MCU_PLL2_HSDIV7_DIV_VAL;
        div_val_8 = MCU_PLL2_HSDIV8_DIV_VAL;
        ssmod_spreadval = MCU_PLL2_SSMOD_SPREAD;
        ssmod_input_div = MCU_PLL2_SSMOD_MODDIV;
        ssmod_spreadtype = MCU_PLL2_SSMOD_DOWNSPREAD;
    }

    if(debug_info){
        printf_("Note: deskew PLL programming isn't implemented yet\n");
    }

    if(PLL_Type == DESKEW_PLL){
        //Program the Deskew PLL
        if(debug_info){
            printf_("Put deskew PLL programming here\n");
        }
    }else{
        //Program the fractional PLL.
        if(debug_info){
            printf_("This is a fractional PLL, continuing on with normal programming.\n");
        }
    }

    //Debugging information for ensuring the GEL is writing to the right areas.
    if(debug_info){
        printf_("For debugging: \n");
        printf_("Base address: %x\n", Base_Address);
        printf_("PLL index: %x\n", PLL_index);
        printf_("PLL index register base: %x\n",(PLL_index * 0x1000));
        printf_("Register: %x\n", CONTROL);
        printf_("Clocking scheme: %d\n",Clocking_Scheme);
    }

    //Put the PLL in external bypass first. Write "1" to bit #31 in the control register.
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 1, 1, 31);
    if(debug_info){
        printf_("Set PLL to external bypass via Control MMR.\n");
    }

    //Disable all HSDIVs
    /*	if(Num_HSDIV){
                    for(i = 0; i < Num_HSDIV; i++){
                            //Clear the enable bit.
                            Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + (i*0x4 + 0x80)), 0, 1, 15);
                            if(debug_info){
                                    printf_("Disabling HSDIV #%d\n", i);
                            }
                    }
            }*/

    //Disable PLL
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 0, 1, 15);
    if(debug_info){
        printf_("Disabled PLL\n");
    }

    //Set DACEN (Enable Factional DAC)
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 1, 1, 0);
    if(debug_info){
        printf_("Enabled noise-cancelling DAC.\n");
    }

    //Set DMEN (Enable Delta-Sigma Modulator)
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 1, 1, 1);

    if(debug_info){
        printf_("Enabled the Delta-Sigma modulator.\n");
    }

    //Set the reference divider value
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + OUTPUT_DIV_CONTROL), frefdiv, 6, 0);
    if(debug_info){
        printf_("Programmed Reference clock pre-divider in output clock divider register.\n");
    }

    //Set the Feedback divider value (12-bit)
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + FREQ_CONTROL_0), fbdiv, 12, 0);
    if(debug_info){
        printf_("Programmed the integer feedback divider value in Freq Control 0 register.\n");
    }

    //Set the fractional multipler/divider value (24-bit)
    if(frac >= 0){ //Sometimes, the fractional divider value will be set to a negative number.
        Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + FREQ_CONTROL_1), frac, 24, 0);
    }else{
        if(debug_info){
            printf_("Fractional Divider Value is -1, don't set the fractional divider.\n");
        }
    }
    if(debug_info){
        printf_("Programmed the fractional feedback divider in Freq Control 0 register.\n");
    }

    //Disable the 4-phase clock generator
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 0, 1, 5);
    if(debug_info){
        printf_("Disabled the 4-phase clock generator (clk_4ph_en) in the control register.\n");
    }

    //Set clk_postdiv_en (enables all the post-divider clocks, 4-phase, and synch. clocks)
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 1, 1, 4);
    if(debug_info){
        printf_("Enabled the FOUT4PHASE clocks in the control register.\n");
    }

    //Set POSTDIV1
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + OUTPUT_DIV_CONTROL), postdiv1, 3, 16);
    if(debug_info){
        printf_("Set the first post-divider value (POSTDIV1) in the output divider control register.\n");
    }

    //Set POSTDIV2
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + OUTPUT_DIV_CONTROL), postdiv2, 3, 24);
    if(debug_info){
        printf_("Set the second post-divider value (POSTDIV2) in the output divider control register.\n");
    }

    //Program HSDIVs
    //HSDIV clocking scheme will already be present.
    if(Num_HSDIV){
        for(i = 0; i < Num_HSDIV; i++){
            //Program the HSDIVs.
            if(debug_info){
                printf_("Programming HSDIV #%d\n", i);
            }
            //hsdiv_value is 0. Need to set it to the right value.
            if(i == 0){
                hsdiv_value = div_val_0;
            }else if(i == 1){
                hsdiv_value = div_val_1;
            }else if(i == 2){
                hsdiv_value = div_val_2;
            }else if(i == 3){
                hsdiv_value = div_val_3;
            }else if(i == 4){
                hsdiv_value = div_val_4;
            }else if(i == 5){
                hsdiv_value = div_val_5;
            }else if(i == 6){
                hsdiv_value = div_val_6;
            }else if(i == 7){
                hsdiv_value = div_val_7;
            }else if(i == 8){
                hsdiv_value = div_val_8;
            }
            if((signed int)hsdiv_value >= (signed int)0){ //If the HSDIV value is -1 then don't program the specific HSDIV.
                Program_HSDIV(Base_Address, address_offset, PLL_index, i, hsdiv_value);
            }else{
                if(debug_info){
                    printf_("i: %d, HSDIV value is -1, don't program this one\n", i);
                }
            }
            if(debug_info){
                printf_("HSDIV #%d programmed.\n", i);
            }
        } //end of iterative loop over all existing HSDIVs
    }

    //Wait 1us
    i = 0; //Clear i just in case.
    for(i = 0; i < 10000; i++);

    //If the PLL has a PLL controller connected to it, configure the PLL controller as well.
    if(PLL_index == 0 || PLL_index == 26){
        if(debug_info){
            if(Base_Address == CSL_PLL0_CFG_BASE){
                printf_("Selected Main Domain PLL Controller.\n");
                //Program_PLLCTRL(CSL_PLLCTRL0_BASE, address_offset);
            }else if(Base_Address == CSL_MCU_PLL0_CFG_BASE){
                printf_("Selected MCU Domain PLL Conntroller.\n");
                //Program_PLLCTRL(CSL_WKUP_PLLCTRL0_BASE, address_offset);
            }
        }
        if((PLL_index == 0) && (Base_Address == CSL_PLL0_CFG_BASE)){
            Program_PLLCTRL(CSL_PLLCTRL0_BASE, address_offset);
        }else if((PLL_index == 0) && (Base_Address == CSL_MCU_PLL0_CFG_BASE)){
            Program_PLLCTRL(CSL_WKUP_PLLCTRL0_BASE, address_offset);
        }
    }

    //Set the PLL enable bit.
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 1, 1, 15);
    if(debug_info){
        printf_("Set the enable bit in the control register.\n");
    }

    //Wait for the lock by polling on the lock bit to be set to "1"
    while(!Read_MMR((Base_Address + address_offset + (PLL_index * 0x1000) + STATUS)));
    //TODO: Need to add a timeout period if the PLL doesn't lock
    if(debug_info){
        printf_("PLL is locked.\n");
    }

    //De-assert the PLL_EXTBYPASS bit to engage the PLL and HSDIV outputs with the rest of the SoC.
    Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CONTROL), 0, 1, 31);
    if(debug_info){
        printf_("External bypass is disabled '0'. PLL and HSDIV clocks are engaging the rest of the SoC.\n");
    }

    //DDR PLL Calibration starts here.
    if(PLL_Type == FRAC_F_PLL){
        //Enable fast calibration
        Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CAL_CONTROL), 1, 1, 20);

        //Set the calibration count to 3 (for slow calibration)
        Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CAL_CONTROL), 3, 3, 16);

        //Set calibration input to 0
        Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CAL_CONTROL), 0, 12, 0);

        //Set calibration bypass to 0
        Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CAL_CONTROL), 0, 1, 15);

        //Enable calibration (set bit 31 to 1)
        Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CAL_CONTROL), 1, 1, 31);

        //Poll on calibration lock
        while(!Read_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CAL_STATUS), 1, 31));

        //Set calibration to the maximum value (CALCNT = 7)
        Write_MMR_Field((Base_Address + address_offset + (PLL_index * 0x1000) + CAL_CONTROL), 7, 3, 16);

        //For debugging
        temp = 0;
        temp = Read_MMR(Base_Address + address_offset + (PLL_index * 0x1000) + CAL_CONTROL);
        printf_("DDR PLL Calibration Control MMR value: %x\n", temp);
        temp = 0;
        temp = Read_MMR(Base_Address + address_offset + (PLL_index * 0x1000) + CAL_STATUS);
        printf_("DDR PLL Calibration Control MMR value: %x\n", temp);

    }
}


static void Turn_On_LPSC_WKUPMCU2MAIN()
{
    int status = 1;
    printf_("Powering up LPSC_WKUPMCU2MAIN\n");
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_WKUPMCU2MAIN, PSC_PD_ON, PSC_ENABLE);
    printf_("Checking LPSC_WKUPMCU2MAIN\n");
    status &= Get_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_WKUPMCU2MAIN);
}

/**
 * \brief   Sets up the PLLs. This will call Program_PLL().
 *
 * \param   Base_Address 		Base address for the PLL MMRs.
 *
 * \param   address_offset 		Address offset for the M3 RAT.
 *
 * \param 	PLL_index 			Index of the PLL to program.
 *
 * \param 	Clocking_Scheme 	The clocking scheme to program the chip for.
 *
 * \return  none
 */
static void Setup(uint32_t Base_Address, uint32_t address_offset, uint32_t PLL_index, uint32_t Clocking_Scheme){
    //Call the Program_PLL() function with the arguments passed in here.
    //Will cover PLLs + HSDIVs + PLLCTRLs + SSMOD
    Program_PLL(Base_Address, address_offset, PLL_index, Clocking_Scheme);
}

/* =========================================================================
 * Public API
 * ========================================================================= */

void Configure_ATCM(void)
{
    REG32(CSL_MCU_SEC_MMR0_CFG0_BASE + 0x100U) = 0x888U;
    REG32(CSL_MCU_SEC_MMR0_CFG0_BASE + 0x180U) = 0x888U;
    REG32(CSL_MAIN_SEC_MMR0_BOOT_CTRL_BASE + 0x100U) = 0x888U;
    REG32(CSL_MAIN_SEC_MMR0_BOOT_CTRL_BASE + 0x180U) = 0x888U;
    REG32(CSL_MAIN_SEC_MMR0_BOOT_CTRL_BASE + 0x1100U) = 0x888U;
    REG32(CSL_MAIN_SEC_MMR0_BOOT_CTRL_BASE + 0x1180U) = 0x888U;
}

void MCU_R5_Cluster_0_split(void) { REG32(CSL_MAIN_SEC_MMR0_BOOT_CTRL_BASE + 0x40U) = 0x00000008U; }

void Set_All_PLL(void)
{
    /* ===================================================================== */
    /* MAIN Domain PLLs -- direct physical addresses, no RAT offset          */
    /* ===================================================================== */
    uint32_t main_offset = 0U;

    // Turn on LPSC_WKUPMCU2MAIN.
    Turn_On_LPSC_WKUPMCU2MAIN();

    // Turn on the PLLs.
    printf_("Programming all PLLs.\n");
    // Main PLL
    printf_("Programming Main PLL 0 (Main PLL)\n");
    Setup(CSL_PLL0_CFG_BASE, main_offset, MAIN_PLL_INDEX, OFC1);
    printf_("Main PLL 0 (Main PLL) Set.\n");
//    // Per0 PLL
//    printf_("Programming Main PLL 1 (Peripheral 0 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, PER0_PLL_INDEX, OFC1);
//    printf_("Main PLL 1 (Peripheral 0 PLL) Set.\n");
//    // Per1 PLL
//    printf_("Programming Main PLL 2 (Peripheral 1 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, PER1_PLL_INDEX, OFC1);
//    printf_("Main PLL 2 (Peripheral 1 PLL) Set.\n");
//    // CPSW9 PLL
//    printf_("Programming Main PLL 3 (CPSW9G PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, CPSW9_PLL_INDEX, OFC1);
//    printf_("Main PLL 3 (CPSW9G PLL) Set.\n");
//    // Audio 0 PLL
//    printf_("Programming Main PLL 4 (Audio 0 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, AUDIO0_PLL_INDEX, OFC1);
//    printf_("Main PLL 4 (Audio 0 PLL) Set.\n");
//    // Video PLL
//    printf_("Programming Main PLL 5 (Video PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, VIDEO_PLL_INDEX, OFC1);
//    printf_("Main PLL 5 (Video PLL) Set.\n");
//    // GPU PLL
//    printf_("Programming Main PLL 6 (GPU PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, GPU_PLL_INDEX, OFC1);
//    printf_("Main PLL 6 (GPU PLL) Set.\n");
//    // C7x PLL
//    printf_("Programming Main PLL 7 (C7x PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, C7X_PLL_INDEX, OFC1);
//    printf_("Main PLL 7 (C7x PLL) Set.\n");
//    // ARM0 PLL
//    printf_("Programming Main PLL 8 (ARM0 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, ARM0_PLL_INDEX, OFC1);
//    printf_("Main PLL 8 (ARM0 PLL) Set.\n");
//    // DDR PLL
//    printf_("Programming Main PLL 12 (DDR PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, DDR_PLL_INDEX, OFC1);
//    printf_("Main PLL 12 (DDR PLL) Set.\n");
//    // C66x PLL
//    printf_("Programming Main PLL 13 (C66x PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, C66_PLL_INDEX, OFC1);
//    printf_("Main PLL 13 (C66x PLL) Set.\n");
//    // Main SoC Pulsar PLL
//    printf_("Programming Main PLL 14 (Main Domain Pulsar) PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, MAIN_R5F_PLL_INDEX, OFC1);
//    printf_("Main PLL 14 (Main Domain Pulsar PLL) Set.\n");
//    // Audio 1 PLL
//    printf_("Programming Main PLL 15 (Audio 1 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, AUDIO1_PLL_INDEX, OFC1);
//    printf_("Main PLL 15 (Audio 1 PLL) Set.\n");
//    // DSS PLLs -- DSS0 PLL
//    printf_("Programming Main PLL 16 (DSS0 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, DSS0_PLL_INDEX, OFC1);
//    printf_("Main PLL 16 (DSS0 PLL) Set.\n");
//    // DSS PLLs -- DSS1 PLL
//    printf_("Programming Main PLL 17 (DSS1 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, DSS1_PLL_INDEX, OFC1);
//    printf_("Main PLL 17 (DSS1 PLL) Set.\n");
//    // DSS PLLs -- DSS2 PLL
//    printf_("Programming Main PLL 18 (DSS2 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, DSS2_PLL_INDEX, OFC1);
//    printf_("Main PLL 17 (DSS2 PLL) Set.\n");
//    // DSS PLLs -- DSS3 PLL
//    printf_("Programming Main PLL 19 (DSS3 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, DSS3_PLL_INDEX, OFC1);
//    printf_("Main PLL 19 (DSS3 PLL) Set.\n");
//    // DSS PLLs -- DSS7 PLL
//    printf_("Programming Main PLL 23 (DSS7 PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, DSS7_PLL_INDEX, OFC1);
//    printf_("Main PLL 23 (DSS7 PLL) Set.\n");
//    // Vision PLL
//    printf_("Programming Main PLL 25 (Vision PLL)\n");
//    Setup(CSL_PLL0_CFG_BASE, main_offset, VISION_PLL_INDEX, DMPAC_520);
//    printf_("Main PLL 25 (Vision PLL) Set.\n");

    /* ===================================================================== */
    /* MCU Domain PLLs -- direct physical addresses, no RAT offset           */
    /* ===================================================================== */
    uint32_t mcu_offset = 0U;

    // MCU PLL
    printf_("Programming MCU PLL 0 (MCU PLL)\n");
    Setup(CSL_MCU_PLL0_CFG_BASE, mcu_offset, MCU_R5F_PLL_INDEX, OFC1);
    printf_("MCU PLL 0 (MCU PLL) Set.\n");
    // MCU Peripheral PLL
//    printf_("Programming MCU PLL 1 (MCU Peripheral PLL)\n");
//    Setup(CSL_MCU_PLL0_CFG_BASE, mcu_offset, MCU_DOM_PLL_INDEX, OFC1);
//    printf_("MCU PLL 1 (MCU PLL) Set.\n");
//    // MCU CPSW PLL
//    printf_("Programming MCU PLL 2 (MCU CPSW PLL)\n");
//    Setup(CSL_MCU_PLL0_CFG_BASE, mcu_offset, MCU_CPSW_PLL_INDEX, OFC1);
//    printf_("MCU PLL 2 (MCU PLL) Set.\n");
//    printf_("All PLLs programmed.\n");
}

int Set_PSC_All_On(void)
{
    int status = 1;

    /* ===================================================================== */
    /* WKUP Domain PSCs                                                       */
    /* ===================================================================== */
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_WKUP_ALWAYSON, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_DMSC, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_DEBUG2DMSC, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_WKUP_GPIO, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_WKUPMCU2MAIN, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_TEST, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_DEBUG, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_MCAN_0, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_MCAN_1, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_OSPI_0, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_OSPI_1, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_HYPERBUS, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_I3C_0, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_I3C_1, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_ADC_0, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_GP_CORE_CTL_WKUP, LPSC_MCU_ADC_1, PSC_PD_ON, PSC_ENABLE);
    /* R5F */
    status &= Set_WKUP_PSC_State(PD_MCU_PULSAR, LPSC_MCU_R5_0, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_MCU_PULSAR, LPSC_MCU_R5_1, PSC_PD_ON, PSC_ENABLE);
    status &= Set_WKUP_PSC_State(PD_MCU_PULSAR, LPSC_MCU_PULSAR_PBIST_0, PSC_PD_ON, PSC_ENABLE);

//    /* ===================================================================== */
//    /* MAIN Domain PSCs                                                       */
//    /* ===================================================================== */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_MAIN_ALWAYSON, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_MAIN_TEST, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_MAIN_PBIST, PSC_PD_ON, PSC_ENABLE);
//    /* Audio */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_AUDIO, PSC_PD_ON, PSC_ENABLE);
//    /* ATL */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_ATL, PSC_PD_ON, PSC_ENABLE);
//    /* MLB */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_MLB, PSC_PD_ON, PSC_ENABLE);
//    /* Motor */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_MOTOR, PSC_PD_ON, PSC_ENABLE);
//    /* MISCIO */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_MISCIO, PSC_PD_ON, PSC_ENABLE);
//    /* GPMC */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_GPMC, PSC_PD_ON, PSC_ENABLE);
//    /* VPFE */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_VPFE, PSC_PD_ON, PSC_ENABLE);
//    /* VPE */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_VPE, PSC_PD_ON, PSC_ENABLE);
//    /* Don't turn on the spare main LPSCs (there are 2 of them) */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_MAIN_DEBUG, PSC_PD_ON, PSC_ENABLE);
//    /* DDR EMIFs */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_EMIF_CFG_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_EMIF_DATA_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_EMIF_CFG_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_EMIF_DATA_1, PSC_PD_ON, PSC_ENABLE);
//    /* DMTimer */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_DMTIMER_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_DMTIMER_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_DMTIMER_2, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_DMTIMER_3, PSC_PD_ON, PSC_ENABLE);
//    /* MMC/SD */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_MMC4B_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_MMC4B_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_MMC8B_0, PSC_PD_ON, PSC_ENABLE);
//    /* UFS */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_UFS_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_UFS_1, PSC_PD_ON, PSC_ENABLE);
//    /* SAUL */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_SAUL, PSC_PD_ON, PSC_ENABLE);
//    /* I3C */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PER_I3C, PSC_PD_ON, PSC_ENABLE);
//    /* MCANSS */
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_2, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_3, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_4, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_5, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_6, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_7, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_8, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_9, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_10, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_11, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_12, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_MCANSS, LPSC_MAIN_MCANSS_13, PSC_PD_ON, PSC_ENABLE);
//    /* SERDES */
//    status &= set_main_psc_state(PD_SERDES_0, LPSC_SERDES_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_SERDES_1, LPSC_SERDES_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_SERDES_2, LPSC_SERDES_2, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_SERDES_3, LPSC_SERDES_3, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_SERDES_4, LPSC_SERDES_4, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_SERDES_5, LPSC_SERDES_5, PSC_PD_ON, PSC_ENABLE);
//    /* USB */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_USB_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_USB_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_USB_2, PSC_PD_ON, PSC_ENABLE);
//    /* PCIe */
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PCIE_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PCIE_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PCIE_2, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GP_CORE_CTRL, LPSC_PCIE_3, PSC_PD_ON, PSC_ENABLE);
//    /* ICSSG */
//    status &= set_main_psc_state(PD_ICSS, LPSC_ICSSG_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_ICSS, LPSC_ICSSG_1, PSC_PD_ON, PSC_ENABLE);
//    /* 9GSS */
//    status &= set_main_psc_state(PD_9GSS, LPSC_9GSS, PSC_PD_ON, PSC_ENABLE);
//    /* DSS */
//    status &= set_main_psc_state(PD_DSS, LPSC_DSS_PBIST, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_DSS, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_TX_DPHY_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_DSI, PSC_PD_ON, PSC_ENABLE);
//    /* eDP */
//    status &= set_main_psc_state(PD_DSS, LPSC_EDP_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_EDP_1, PSC_PD_ON, PSC_ENABLE);
//    /* CSI */
//    status &= set_main_psc_state(PD_DSS, LPSC_CSITX_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_CSIRX_PHY_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_CSIRX_PHY_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_CSIRX_PHY_2, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_CSIRX_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_CSIRX_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DSS, LPSC_CSIRX_2, PSC_PD_ON, PSC_ENABLE);
//    /* C7x */
//    status &= set_main_psc_state(PD_C71X_0, LPSC_C71X_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_C71X_1, LPSC_C71X_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_C71X_0, LPSC_C71X_0_PBIST, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_C71X_0, LPSC_C71X_1_PBIST, PSC_PD_ON, PSC_ENABLE);
//    /* A72 */
//    status &= set_main_psc_state(PD_A72_CLUSTER_0, LPSC_A72_CLUSTER_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_A72_CLUSTER_0, LPSC_A72_CLUSTER_0_PBIST, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_A72_0, LPSC_A72_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_A72_1, LPSC_A72_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_A72_CLUSTER_1, LPSC_A72_CLUSTER_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_A72_CLUSTER_1, LPSC_A72_CLUSTER_1_PBIST, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_A72_2, LPSC_A72_2, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_A72_3, LPSC_A72_3, PSC_PD_ON, PSC_ENABLE);
//    /* GPU */
//    status &= set_main_psc_state(PD_GPUCOM, LPSC_GPUCOM, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GPUCOM, LPSC_GPUPBIST, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_GPUCORE, LPSC_GPUCORE, PSC_PD_ON, PSC_ENABLE);
//    /* C66x */
//    status &= set_main_psc_state(PD_C66X_0, LPSC_C66X_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_C66X_0, LPSC_C66X_PBIST_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_C66X_1, LPSC_C66X_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_C66X_1, LPSC_C66X_PBIST_1, PSC_PD_ON, PSC_ENABLE);
//    /* MAIN SoC R5F */
//    status &= set_main_psc_state(PD_PULSAR_0, LPSC_PULSAR_0_R5_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_PULSAR_0, LPSC_PULSAR_0_R5_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_PULSAR_0, LPSC_PULSAR_0_PBIST, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_PULSAR_1, LPSC_PULSAR_1_R5_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_PULSAR_1, LPSC_PULSAR_1_R5_1, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_PULSAR_1, LPSC_PULSAR_1_PBIST, PSC_PD_ON, PSC_ENABLE);
//    /* Decode */
//    status &= set_main_psc_state(PD_DECODE, LPSC_DECODE_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DECODE, LPSC_DECODE_PBIST, PSC_PD_ON, PSC_ENABLE);
//    /* Encode */
//    status &= set_main_psc_state(PD_ENCODE, LPSC_ENCODE_0, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_ENCODE, LPSC_ENCODE_PBIST, PSC_PD_ON, PSC_ENABLE);
//    /* DMPAC */
//    status &= set_main_psc_state(PD_DMPAC, LPSC_DMPAC, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DMPAC, LPSC_SDE, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_DMPAC, LPSC_DMPAC_PBIST, PSC_PD_ON, PSC_ENABLE);
//    /* VPAC */
//    status &= set_main_psc_state(PD_VPAC, LPSC_VPAC, PSC_PD_ON, PSC_ENABLE);
//    status &= set_main_psc_state(PD_VPAC, LPSC_VPAC_PBIST, PSC_PD_ON, PSC_ENABLE);

    return status;
}


void j721e_early_init(void)
{
    Configure_ATCM();
    MCU_R5_Cluster_0_split();
    Set_All_PLL();
    Set_PSC_All_On();
}

