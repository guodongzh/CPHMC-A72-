/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_init.h
 *@author     LiuRui
 *@date       2024.11.05
 *@brief      J721E PCIE RC
 *@par        History
 *Date        Version   Author     Description
 *2024.11.05  1.0       LiuRui     first version
 ******************************************************************************/

#ifndef PCIE_INIT_H_
#define PCIE_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <string.h>
#include <ti/csl/cslr_device.h>
#include <ti/drv/pcie/pcie.h>
#include "pcie_xdma.h"
#include <ti/drv/uart/UART_stdio.h>
#include "memory_map_defines.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/* all addresses are 32bit */

#define INV_ADDR 0xffffffff

#define PCIE0_IB0_BASE  (0x4D80838000UL) 
#define PCIE0_IB0_SIZE  (1024 * 16 * 1)   /* 16KB */

#define PCIE0_IB1_BASE  (0x00838000) 
#define PCIE0_IB1_SIZE (1024 * 16 * 1)   /* 16KB */

/**
 * @{ PCIE0
 */
/* Outbound PCIe Base Address for RC*/
#define PCIE0_OB_LO_XDMA_RC     0x20000000  // fpga dma bar
#define PCIE0_OB_XDMA_BYPASS_RC 0x30000000  // fpga dma bypass

/* Cfg area offset absolute including PCIe base*/
#define PCIE0_CFG_BASE          (CSL_PCIE0_DAT0_BASE + 0x00010000U)
#define PCIE0_CFG_MASK          0x0000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE0_AXI_LITE_BASE     (CSL_PCIE0_DAT0_BASE + 0x01000000U)
#define PCIE0_AXI_LITE_MASK     0x0000FFFFFU

/* Data area offset absolute including PCIe base */
#define PCIE0_XDMA_BASE         (CSL_PCIE0_DAT0_BASE + 0x02000000U)
#define PCIE0_XDMA_MASK         0x000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE0_XDMA_BYPASS_BASE  (CSL_PCIE0_DAT0_BASE + 0x03000000U)
#define PCIE0_XDMA_BYPASS_MASK  0x000FFFFU

#define PCIE0_IB1_RAM_ADDR       0x004d80838000
#define PCIE0_IB1_PCIE_ADDR      0x80838000

/**
 * @}
 */

/**
 * @{ PCIE1
 */
/* Outbound PCIe Base Address for RC*/
#define PCIE1_OB_LO_XDMA_RC     0x20000000  // fpga dma bar
#define PCIE1_OB_XDMA_BYPASS_RC 0x30000000  // fpga dma bypass

/* Cfg area offset absolute including PCIe base*/
#define PCIE1_CFG_BASE          (CSL_PCIE1_DAT0_BASE + 0x00010000U)
#define PCIE1_CFG_MASK          0x0000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE1_AXI_LITE_BASE     (CSL_PCIE1_DAT0_BASE + 0x01000000U)
#define PCIE1_AXI_LITE_MASK     0x0000FFFFFU

/* Data area offset absolute including PCIe base */
#define PCIE1_XDMA_BASE         (CSL_PCIE1_DAT0_BASE + 0x02000000U)
#define PCIE1_XDMA_MASK         0x000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE1_XDMA_BYPASS_BASE  (CSL_PCIE1_DAT0_BASE + 0x03000000U)
#define PCIE1_XDMA_BYPASS_MASK  0x000FFFFU

/**
 * @}
 */

/**
 * @{ PCIE2
 */

#define PCIE2_OB_LO_XDMA_RC     0x20000000    // fpga dma bar
#define PCIE2_OB_XDMA_BYPASS_RC 0x30000000  // fpga dma bypass

/* Cfg area offset absolute including PCIe base*/
#define PCIE2_CFG_BASE          (CSL_PCIE2_DAT0_BASE + 0x00010000U)
#define PCIE2_CFG_MASK          0x0000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE2_AXI_LITE_BASE     (CSL_PCIE2_DAT0_BASE + 0x01000000U)
#define PCIE2_AXI_LITE_MASK     0x0000FFFFFU

/* Data area offset absolute including PCIe base */
#define PCIE2_XDMA_BASE         (CSL_PCIE2_DAT0_BASE + 0x02000000ULL)
#define PCIE2_XDMA_MASK         0x000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE2_XDMA_BYPASS_BASE  (CSL_PCIE2_DAT0_BASE + 0x03000000ULL)
#define PCIE2_XDMA_BYPASS_MASK  0x000FFFFU

/**
 * @]
 */


/**
 * @{ PCIE3
 */

#define PCIE3_OB_LO_XDMA_RC     0x20000000  // fpga dma bar
#define PCIE3_OB_XDMA_BYPASS_RC 0x30000000  // fpga dma bypass

/* Cfg area offset absolute including PCIe base*/
#define PCIE3_CFG_BASE          (CSL_PCIE3_DAT0_BASE + 0x00010000U)
#define PCIE3_CFG_MASK          0x0000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE3_AXI_LITE_BASE     (CSL_PCIE3_DAT0_BASE + 0x01000000U)
#define PCIE3_AXI_LITE_MASK     0x0000FFFFFU

/* Data area offset absolute including PCIe base */
#define PCIE3_XDMA_BASE         (CSL_PCIE3_DAT0_BASE + 0x02000000ULL)
#define PCIE3_XDMA_MASK         0x000FFFFU

/* Data area offset absolute including PCIe base */
#define PCIE3_XDMA_BYPASS_BASE  (CSL_PCIE3_DAT0_BASE + 0x03000000ULL)
#define PCIE3_XDMA_BYPASS_MASK  0x000FFFFU

/**
 * @]
 */


/* Inbound PCIe Base Address for RC*/
#define PCIE_IB0_LO_ADDR_RC     PCIE0_IB_BASE
#define PCIE_IB0_HI_ADDR_RC     0

/* Inbound limit */
/* uint8_t ib0_space[1024 * 1024 * 32]; Documented only */
#define PCIE0_INBOUND0_MASK0     0x00003FFFU
#define PCIE0_INBOUND0_MASK      0x000FFFFFU
#define PCIE1_INBOUND0_MASK      0x000FFFFFU
#define PCIE2_INBOUND0_MASK      0x000FFFFFU
#define PCIE3_INBOUND0_MASK      0x000FFFFFU

/* BAR Index PCie*/
#define PCIE_BAR_IDX_RC         0

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

#define CACHE_LINE_SIZE         128

#define PCIE_BYTES              ((sizeof(struct xdma_desc) * (XDMA_CHANNEL_NUM_MAX * 2)) + MAX_PKT_LEN * 2 * 4)
#define PCIE_REM                (PCIE_BYTES % CACHE_LINE_SIZE)
#define PCIE_IB_PAD             (PCIE_REM ? (CACHE_LINE_SIZE - PCIE_REM) : 0)

typedef struct ib_buff
{
    struct xdma_desc xdma_sg_desc_h2c[XDMA_CHANNEL_NUM_MAX];
    struct xdma_desc xdma_sg_desc_c2h[XDMA_CHANNEL_NUM_MAX];
    uint32_t         rx_buffer[MAX_PKT_LEN];
    uint32_t         tx_buffer[MAX_PKT_LEN];
    uint8_t          padding[PCIE_IB_PAD];
} ib_buff_t;

extern uint8_t pcie0_ib_space[1024 * 1024 * 1];
extern uint8_t pcie2_ib_space[1024 * 1024 * 1];
extern uint8_t pcie3_ib_space[1024 * 1024 * 1];

extern uint8_t pcie0_ib1_space[1024 * 16 *  1];  
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

pcieRet_e pcie_init(uint32_t device_num);

#ifdef __cplusplus
}
#endif

#endif
