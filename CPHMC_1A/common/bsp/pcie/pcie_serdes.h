/**
 * @file pcie_example_board.h
 *
 */

#ifndef _PCIE_SAMPLE_BOARD_H_
#define _PCIE_SAMPLE_BOARD_H_

#include <ti/osal/osal.h>

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

uint32_t main_mmr_unlock_all();
void pcie_serdes_cfg(int32_t serdes);
void pcie_set_mode(uint32_t ints_num, uint32_t rate,
                   uint32_t mode, uint32_t lane_count);
void pcie_refclk_to_io(uint32_t ints_num, uint32_t ref_clk);

#endif

