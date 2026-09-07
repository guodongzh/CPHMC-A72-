
/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "norflash_fw_writer.h"
uint8_t FW_BootFlag = 0;

#ifdef BUILD_MCU2_0

#include <ti/drv/sciclient/sciclient.h>
#include "norflash_fw_writer.h"
#include "board/src/flash/include/board_flash.h"
#include "ti/drv/spi/SPI.h"
#include "ti/drv/spi/soc/SPI_soc.h"
#include "flash.h"
#include "net_tcp.h"



/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
#define FLASH_ID         BOARD_FLASH_ID_IS25LP128F
#define FLASH_PORT       0

/* OSPI NOR flash offset address for read/write test */
#define TEST_ADDR_OFFSET (0U)

/* Test read/write buffer length in bytes */
#define TEST_BUF_LEN     (0x100000U)

/* Buffer containing he known data that needs to be written to flash */
uint8_t gTxBuf[TEST_BUF_LEN];

/* Buffer containing the received data */
uint8_t gRxBuf[TEST_BUF_LEN];

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */
/* Function to generate known data */

uint8_t gFWDDRFileBuf[FW_MAX_FILE_SIZE] __attribute__((aligned(128), section(".filebuf"))) = {0};
uint8_t *gFwDDRbufPtr = gFWDDRFileBuf;
uint32_t fw_file_szie_tmp = 0;
uint32_t fw_file_szie = 0;

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
static void flash_write_firmware(void)
{
    int32_t status = BOARD_FLASH_EOK;
    uint32_t offset = Core0_APP_ADDR;
    Board_FlashInfo *flash_info = (Board_FlashInfo *)flash_ctrl0_handle;
    uint32_t sector_num, page_num;
    uint32_t sector_num_of_single = fw_file_szie % flash_info->sector_size;
    uint32_t sector_count = fw_file_szie / flash_info->sector_size;
    uint8_t *write_data_ptr = gFWDDRFileBuf;
    uint32_t sector_size = flash_info->sector_size;

    if (sector_num_of_single)
    {
        sector_count += 1;
    }

    FLASH_log("write flash addr=0x%08x\n", offset);

    if (g_pscode_udp_info.pscode_src_port == 5000)
        offset = Core0_APP_ADDR;
    else if (g_pscode_udp_info.pscode_src_port == 5001)
        offset = Core1_APP_ADDR;
    else if (g_pscode_udp_info.pscode_src_port == 5002)
        offset = Core2_APP_ADDR;
    else if (g_pscode_udp_info.pscode_src_port == 5003)
        offset = Core3_APP_ADDR;
    else if (g_pscode_udp_info.pscode_src_port == 5004)
        offset = Core4_APP_ADDR;
    else if (g_pscode_udp_info.pscode_src_port == 5005)
        offset = Core5_APP_ADDR;
    else if (g_pscode_udp_info.pscode_src_port == 5006)
        offset = Core6_APP_ADDR;
    else if (g_pscode_udp_info.pscode_src_port == 5007)
        offset = Core7_App_ADDR;
    else
    {
        Debug_log("Invalid port number!!!\n");
        return;
    }   
        
    /* Erase the corresponding sectors according to the amount of data to be written */
    for (uint32_t i = 0; i < sector_count; i++)
    {
        /*get sector number and page number*/
        status = Board_flashOffsetToSectorPage(flash_ctrl0_handle, offset, &sector_num, &page_num);
        if (status != BOARD_FLASH_EOK)
        {
            Debug_log("Board_flashOffsetToSectorPage failed!!!\n");
        }

        FLASH_log("Erase SectorNum: %d\n", sector_num);

        /* Erase sector which data has to be written */
        status = Board_flashEraseSector(flash_ctrl0_handle, sector_num, 0);
        if (status != BOARD_FLASH_EOK)
        {
            Debug_log("Board_flashEraseSector failed!!!\n");
        }

        /* Write Firmware to flash */
        FLASH_log("Write SectorNum: %d bufptr=0x%p,", sector_num, write_data_ptr);
        status = Board_flashWrite(flash_ctrl0_handle, offset, write_data_ptr, sector_size, 0);
        if (status != BOARD_FLASH_EOK)
        {
            Debug_log("Write %d bytes failed at 0x%X offset !!!\n", sector_size, offset);
            if (i != 0)
                i--;
            continue;
        }

        /* Read buffer from flash */
        if (Board_flashRead(flash_ctrl0_handle, offset, &gRxBuf[0], sector_size, 0))
        {
            Debug_log("Board_flashRead failed. \n");
        }
        /* Verify Data */
        if (verify_data(write_data_ptr, gRxBuf, sector_size) == false)
        {
            Debug_log("Data mismatch. \n");
        }
        else
        {
            FLASH_log("programming completed!\n");
        }
        offset += sector_size;
        write_data_ptr += sector_size;
    }
}

void FW_writeDDR(const void *pBuf, int size, int lastFlag)
{
    uint8_t coreid = 0;
    memcpy(gFwDDRbufPtr, pBuf, size);
    gFwDDRbufPtr += size;
    fw_file_szie_tmp = fw_file_szie_tmp + size;

    if (lastFlag == 0xFF)
    {
        gFwDDRbufPtr = gFWDDRFileBuf;
        fw_file_szie = fw_file_szie_tmp;

        fw_file_szie_tmp = 0;

        FLASH_log("Debug(R%d): Received firmware size =%d\n", coreid, fw_file_szie);

        if (fw_file_szie > FW_MAX_FILE_SIZE)
        {
            FLASH_log("Error(R%d): Received Firmware oversize!!!\n", coreid);
        }
        else
        {
            Debug_log("download fw start\n");
            flash_write_firmware();
            Debug_log("download fw over\n");
        }
    }
}

#endif
