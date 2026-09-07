/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       platform .c
*@author     jinyangh
*@date       2025.10.30
*@brief
*@par        History
*Date        Version   Author     Description
*2025.10.30  1.0       jinyangh    example
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "platform.h"
#include "file_system.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
/*************************************** board ***************************************/
uint8_t GPEMCSlotID = 0;

/*************************************** pcie ****************************************/
train_tx_t  *enet_tx_cfg = &slot2_train_tx[SLOW_CAR_ID];
train_tx_t  *irig_b_train_tx = &slot0_1_train_tx[9];
train_rx_t  *irig_b_train_rx = &slot0_1_train_rx[3];

/*************************************** flash ***************************************/
Flash_Handle flash_handle0 = (void *)&flash_ctrl0_handle;
Flash_Handle flash_handle1 = (void *)&flash_ctrl1_handle;
Flash_Handle flash_handle2 = (void *)&flash_ctrl1_handle;

/*************************************** file system *********************************/
Flash_Handle filesystem_flash_handle              = NULL;
char fatfile_keyword[FILE_KEY_NUM][FILE_NAME_LEN] = {
    "sbl",
    "tifs",
    "tbl",
    "atf_optee_spl",
    "R5F0",
    "R5F1",
    "R5F2",
    "R5F3",
    "DSPC6X0",
    "DSPC6X1",
    "DSPC7X0",
    "MCU1",
    "all_cores",
    "cphmc_top",
};

char fatfile_type[FILE_KEY_NUM][FILE_NAME_LEN]    = {
    ".tiimage",
    ".bin",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".appimage.hs_fs",
    ".bin",
};

/*************************************** report manage *******************************/
Flash_Handle report_manage_flash_handle           = NULL;

/*************************************** setting manage ******************************/
Flash_Handle setting_manage_flash_handle          = NULL;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/*************************************** core ****************************************/
uint8_t getCoreNr(void)
{
#if defined(BUILD_MCU2_0)
    return 0;
#elif defined(BUILD_MCU2_1)
    return 1;
#elif defined(BUILD_MCU3_0)
    return 2;
#elif defined(BUILD_MCU3_1)
    return 3;
#elif defined(BUILD_C66X_1)
    return 4;
#elif defined(BUILD_C66X_2)
    return 5;
#elif defined(BUILD_C7X_1)
    return 6;
#elif defined(BUILD_MCU1_1)
    return 7;
#else
    return 0xFF;
#endif
}

/*************************************** cache ***************************************/
void cache_inv_com(void *blockPtr, uint32_t byteCnt, uint32_t type)
{
    CacheP_Inv(blockPtr, byteCnt);
}
void cache_wb_com(void *addr, uint32_t size, uint32_t type)
{
    CacheP_wb(addr, size);
}
#ifdef BUILD_MCU
/*************************************** flash ***************************************/
int32_t Flash_read_com(void *handle, uint32_t offset, uint8_t *buf, uint32_t len, uint8_t flash_chipSelect)
{
    return Board_flashRead((*(Board_flashHandle *)handle), offset, buf, len, flash_chipSelect);
}
int32_t Flash_write_com(void *handle, uint32_t offset, uint8_t *buf, uint32_t len, uint8_t flash_chipSelect)
{
    return Board_flashWrite((*(Board_flashHandle *)handle), offset, buf, len, flash_chipSelect);
}
int32_t Flash_eraseSector_com(void *handle, uint32_t sectorNum, uint8_t flash_chipSelect)
{
    return Board_flashEraseSector((*(Board_flashHandle *)handle), sectorNum, flash_chipSelect);
}
int32_t Flash_offsetToSectorPage_com(void *handle, uint32_t offset, uint32_t *sector, uint32_t *page)
{
    return Board_flashOffsetToSectorPage((*(Board_flashHandle *)handle), offset, sector, page);
}

/*************************************** gpio ****************************************/
void GPIO_pinWriteLow_com(uint32_t baseAddr, uint32_t pinNum)
{

}
void GPIO_pinWriteHigh_com(uint32_t baseAddr, uint32_t pinNum)
{

}

#endif
