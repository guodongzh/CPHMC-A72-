/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       sd_power.h
 *@author     LiuRui
 *@date       2024.09.06
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.09.06  1.0       LiuRui
 ******************************************************************************/

#ifndef _SBL_OSPI_SD_POWER_H
#define _SBL_OSPI_SD_POWER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define GPIO_SD_PIN_NUM   111 /* Pin 111 */
#define GPIO_SD_PORT_NUM  0 /* use MAIN GPIO0 */
#define GPIO_SD_PW_IDX    0

#define CSL_GPIOMUX_INTRTR0_MUXCNTL_START   (0x00a00004U)
#define CSL_GPIOMUX_INTRTR0_MUXCNTL_ENABLE  (1 << 16)

#define CSL_C66SS0_INTRTR0_MUXCNTL_START    (0x00AC0004U)
#define CSL_C66SS0_INTRTR0_MUXCNTL_ENABLE   (1 << 16)

#define CSL_C66SS1_INTRTR0_MUXCNTL_START    (0x00AD0004U)
#define CSL_C66SS1_INTRTR0_MUXCNTL_ENABLE   (1 << 16)

/* define the unlock and lock values */
#define KICK0_UNLOCK_VAL        0x68EF3490
#define KICK1_UNLOCK_VAL        0xD172BC5A
#define KICK_LOCK_VAL           0x00000000

#define MAIN_MMR_BASE_ADDRESS   CSL_CTRL_MMR0_CFG0_BASE

#define MAIN_CTRL_ACSPCIE0_CTRL (0x18090)
#define MAIN_CTRL_ACSPCIE1_CTRL (0x18094)

typedef enum
{
    PCIE_GEN1 = 0,
    PCIE_GEN2,
    PCIE_GEN3,
} SERDES_DIAG_PCIE_TYPE;

typedef enum
{
    PCIE_X1 = 0,
    PCIE_X2,
} PCIE_LANE_COUNT;

typedef enum
{
    PCIE_EP = 0,
    PCIE_RC,
} PCIE_MODE;


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
uint32_t main_mmr_unlock_all();
void pcie_refclk_to_io(uint32_t ints_num, uint32_t ref_clk);
void pcie_set_mode(uint32_t ints_num, uint32_t rate,
                   uint32_t mode, uint32_t lane_count);
void enable_sd_power(void);
void gpio_intr_init();



#ifdef __cplusplus
}
#endif

#endif //_SBL_OSPI_SD_POWER_H