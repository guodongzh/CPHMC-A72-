/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       flash.c
*@author     LiuRui
*@date       2024.08.22
*@brief      nor flash read and write
*@par        History
*Date        Version   Author     Description
*2024.08.22  1.0       LiuRui     first version
*2025.09.11  2.0       LiuRui     second version
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "flash.h"
#include "bsp_init.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
/* Test read/write buffer length in bytes */
#define TEST_BUF_LEN     (0x100000U)

/* flash diagnostic */
flash_diag_t flash_diag = {0};

/* Buffer containing he known data that needs to be written to flash */
uint8_t gTxBuf[TEST_BUF_LEN];

/* Buffer containing the received data */
uint8_t gRxBuf[TEST_BUF_LEN];

Board_flashHandle flash_ctrl0_handle;
Board_flashHandle flash_ctrl1_handle;

static void generate_pattern(uint8_t *txBuf, uint8_t *rxBuf, uint32_t length)
{
    volatile uint32_t idx;
    volatile uint8_t *txPtr = txBuf;
    volatile uint8_t *rxPtr = rxBuf;

    for (idx = 0; idx < length; idx++)
    {
        if (idx < (length / 2))
        {
            *txPtr++ = (uint8_t)idx;
        }
        else if (idx < (length / 4 * 3))
        {
            *txPtr++ = 0xaa;
        }
        else
        {
            *txPtr++ = 0x55;
        }
        *rxPtr++ = (uint8_t)0U;
    }
}

bool verify_data(uint8_t *expData, uint8_t *rxData, uint32_t length)
{
    uint32_t idx = 0;
    uint32_t match = 1;
    bool retVal = false;

    for (idx = 0; ((idx < length) && (match != 0)); idx++)
    {
        if (*expData != *rxData)
        {
            match = 0;
            FLASH_log("Data mismatch at idx %d\n", idx);
        }
        expData++;
        rxData++;
    }

    if (match == 1)
    {
        retVal = true;
    }

    return retVal;
}

/**
 * @brief config ospi clk to freq
 * @param freq can be one of flow value
 *          OSPI_MODULE_CLK_133M
 *          OSPI_MODULE_CLK_166M
 *          OSPI_MODULE_CLK_160M
 *          OSPI_MODULE_CLK_200M
 */
void OSPI_configClk(uint32_t freq, uint32_t port_num)
{
    OSPI_v0_HwAttrs ospi_cfg;
    int32_t retVal;
    uint64_t ospi_rclk_freq;
    uint32_t parClk;

    uint32_t clkID[] = {TISCI_DEV_MCU_FSS0_OSPI_0_OSPI_RCLK_CLK,
                        TISCI_DEV_MCU_FSS0_OSPI_1_OSPI_RCLK_CLK};

    uint32_t devID[] = {TISCI_DEV_MCU_FSS0_OSPI_0, TISCI_DEV_MCU_FSS0_OSPI_1};

    /* Get the default SPI init configurations */
    OSPI_socGetInitCfg(BOARD_OSPI_DOMAIN, port_num, &ospi_cfg);

    /* request setting ospi clk */
    retVal = Sciclient_pmModuleClkRequest(devID[port_num],
                                          clkID[port_num],
                                          TISCI_MSG_VALUE_CLOCK_SW_STATE_REQ,
                                          TISCI_MSG_FLAG_AOP,
                                          SCICLIENT_SERVICE_WAIT_FOREVER);
    if (retVal != CSL_PASS)
    {
        FLASH_log("Sciclient_pmModuleClkRequest failed\n");
        goto clk_cfg_exit;
    }

    /* Max clocks */
    if (freq == OSPI_MODULE_CLK_166M)
    {
        parClk = TISCI_DEV_MCU_FSS0_OSPI_0_OSPI_RCLK_CLK_PARENT_HSDIV4_16FFT_MCU_2_HSDIVOUT4_CLK;

        /* request setting ospi clk */
        retVal = Sciclient_pmSetModuleClkParent(devID[port_num],
                                                clkID[port_num],
                                                parClk,
                                                SCICLIENT_SERVICE_WAIT_FOREVER);
    }
    else
    {
        parClk = TISCI_DEV_MCU_FSS0_OSPI_0_OSPI_RCLK_CLK_PARENT_HSDIV4_16FFT_MCU_1_HSDIVOUT4_CLK;
        retVal = Sciclient_pmSetModuleClkParent(devID[port_num],
                                                clkID[port_num],
                                                parClk,
                                                SCICLIENT_SERVICE_WAIT_FOREVER);
    }

    if (retVal != CSL_PASS)
    {
        FLASH_log("Sciclient_pmSetModuleClkParent failed\n");
        goto clk_cfg_exit;
    }

    ospi_cfg.funcClk = freq;
    OSPI_socSetInitCfg(BOARD_OSPI_DOMAIN, port_num, &ospi_cfg);

    ospi_rclk_freq = (uint64_t)freq;
    retVal = Sciclient_pmSetModuleClkFreq(devID[port_num],
                                          clkID[port_num],
                                          ospi_rclk_freq,
                                          TISCI_MSG_FLAG_AOP,
                                          SCICLIENT_SERVICE_WAIT_FOREVER);

    if (retVal != CSL_PASS)
    {
        FLASH_log("Sciclient_pmSetModuleClkFreq failed\n");
        goto clk_cfg_exit;
    }

    ospi_rclk_freq = 0;
    retVal = Sciclient_pmGetModuleClkFreq(devID[port_num],
                                          clkID[port_num],
                                          &ospi_rclk_freq,
                                          SCICLIENT_SERVICE_WAIT_FOREVER);
    if (retVal != CSL_PASS)
    {
        FLASH_log("Sciclient_pmGetModuleClkFreq failed\n");
        goto clk_cfg_exit;
    }

clk_cfg_exit:
    return;
}

void OSPI_initConfig(uint32_t funcClk, uint32_t port_num)
{
    OSPI_v0_HwAttrs ospi_cfg;

    OSPI_socInit();

    /* Get the default OSPI init configurations */
    OSPI_socGetInitCfg(BOARD_OSPI_DOMAIN, port_num, &ospi_cfg);

    ospi_cfg.intrEnable = false;
    ospi_cfg.dmaEnable = false;
    ospi_cfg.dacEnable = false;
    ospi_cfg.phyEnable = false;
    ospi_cfg.dtrEnable = false;
    ospi_cfg.xferLines = OSPI_XFER_LINES_QUAD;
    ospi_cfg.funcClk = funcClk;
    ospi_cfg.devDelays[0] = 0;
    ospi_cfg.devDelays[1] = 0;
    ospi_cfg.devDelays[2] = 0;
    ospi_cfg.devDelays[3] = 0;
    if (port_num == 0)
    {
        ospi_cfg.baudRateDiv = 4;
    }
    else
    {
        ospi_cfg.baudRateDiv = 8;
    }
    uint32_t clk = funcClk/ospi_cfg.baudRateDiv;
    FLASH_log("OSPI port %d RCLK running at %d MHz. \n",port_num, clk);
    /* Set the default OSPI init configurations */
    OSPI_socSetInitCfg(BOARD_OSPI_DOMAIN, port_num, &ospi_cfg);
}

int flash_interface_init(void)
{
    /* Init OSPI driver module*/
    OSPI_init();

    OSPI_configClk(OSPI_MODULE_CLK_133M, 0);
    OSPI_configClk(OSPI_MODULE_CLK_133M, 1);
    OSPI_initConfig(OSPI_MODULE_CLK_133M, 0);
    OSPI_initConfig(OSPI_MODULE_CLK_133M, 1);

    /* Open the Board OSPI NOR device with OSPI port 0 and 1
       and use default OSPI configurations */
#if defined(BUILD_MCU2_0) || defined(BUILD_MCU1_0)

#else
    int try_cnt = 5;
    CacheP_Inv(&init_flag, 128);
    while (try_cnt)
    {
        if (init_flag.flash_init_ok == INIT_FLGA_MAGIC)
        {
            break;
        }
        Osal_delay(1000);
        CacheP_Inv(&init_flag, 128);
        try_cnt--;
    }
    if (try_cnt == 0)
    {
        Debug_logError("flash init timeout\n");
    }
#endif
    flash_ctrl0_handle = Board_flashOpen(0, NULL);
    flash_ctrl1_handle = Board_flashOpen(1, NULL);

    if (!flash_ctrl0_handle)
    {
        FLASH_log("Board_flashOpen 0 failed. \n");
        return -1;
    }
    NOR_Info flashInfo = ((NOR_Obj *)(((Board_FlashInfo *)flash_ctrl0_handle)->flashHandle))->info;
    Debug_log("\n");
    FLASH_log("NOR Information:\n");
    FLASH_log("\t information device ID: 0x%x, manufacturer ID: 0x%x\n",
              flashInfo.deviceId,
              flashInfo.manufacturerId);
    FLASH_log("\t page_size: %d bytes; sector_size: %d bytes\n",
              flashInfo.pageSize,
              flashInfo.sectorSize);
    FLASH_log("\t block_count: %d; block_size: %d pages = %d sectors  = %d bytes\n",
              flashInfo.blockCnt,
              flashInfo.pageCnt,
              flashInfo.pageCnt * flashInfo.pageSize / flashInfo.sectorSize,
              flashInfo.pageCnt * flashInfo.pageSize);
    FLASH_log("\t spare_size: %d bytes = %d MB\n",
              flashInfo.totalSize,
              flashInfo.totalSize / 1024 / 1024);

    if (!flash_ctrl1_handle)
    {
        FLASH_log("\n Board_flashOpen 1 failed. \n");
        return -1;
    }

    flashInfo = ((NOR_Obj *)(((Board_FlashInfo *)flash_ctrl1_handle)->flashHandle))->info;
    Debug_log("\n");
    FLASH_log("NOR Information:\n");
    FLASH_log("\t information device ID: 0x%x, manufacturer ID: 0x%x\n",
              flashInfo.deviceId,
              flashInfo.manufacturerId);
    FLASH_log("\t page_size: %d bytes; sector_size: %d bytes\n",
              flashInfo.pageSize,
              flashInfo.sectorSize);
    FLASH_log("\t block_count: %d; block_size: %d pages = %d sectors  = %d bytes\n",
              flashInfo.blockCnt,
              flashInfo.pageCnt,
              flashInfo.pageCnt * flashInfo.pageSize / flashInfo.sectorSize,
              flashInfo.pageCnt * flashInfo.pageSize);
    FLASH_log("\t spare_size: %d bytes = %d MB\n",
              flashInfo.totalSize,
              flashInfo.totalSize / 1024 / 1024);
#if defined(BUILD_MCU2_0)
    init_flag.flash_init_ok = INIT_FLGA_MAGIC;
    CacheP_wb(&init_flag, 128);
#endif
    return 0;
}

void flash_test(void)
{
    NOR_Info flashInfo;
    uint32_t sector_num, page_num;
    uint32_t test_len;
    uint32_t start_offset = 0xD80000;
    Board_flash_STATUS status;
    Board_flashHandle flash_handle = flash_ctrl1_handle;
    uint8_t flash_cs = 0;

    flash_diag.wr_rd_running = 1;
    flash_diag.status = 2;
    FLASH_log("spi flash write and read test\r\n");

    if (!flash_handle)
    {
        FLASH_log("Board_flashOpen 0 failed. \n");
        return;
    }
    flashInfo = ((NOR_Obj *)(((Board_FlashInfo *)flash_handle)->flashHandle))->info;  
    test_len = flashInfo.sectorSize;

    for (int j = 0; j < 50; ++j)
    {
        /* Erase sector, to which data has to be written */
        generate_pattern(gTxBuf, gRxBuf, test_len);

        /* Erase sector that data has to be written */
        Board_flashOffsetToSectorPage(flash_handle, start_offset, &sector_num, &page_num);
        FLASH_log("sectorNum = %d ,", sector_num);
        status = Board_flashEraseSector(flash_handle, sector_num, flash_cs);
        if (status != BOARD_FLASH_EOK)
        {
            FLASH_log("Board_flashErase failed. status: %d\n", status);
            flash_diag.wr_rd_err++;
            flash_diag.status = 2;

            goto err;
        }

        /* Write buffer to flash */
        if (Board_flashWrite(flash_handle, start_offset, &gTxBuf[0], test_len, flash_cs))
        {
            Debug_logError("Board_flashWrite failed. \n");
            flash_diag.wr_rd_err++;
            flash_diag.status = 4;
            goto err;
        }

        /* Read buffer from flash */
        if (Board_flashRead(flash_handle, start_offset, &gRxBuf[0], test_len, flash_cs))
        {
            Debug_logError("Board_flashRead failed. \n");
            flash_diag.wr_rd_err++;
            flash_diag.status = 4;
            goto err;
        }

        /* Verify Data */
        if (verify_data(gTxBuf, gRxBuf, test_len) == false)
        {
            Debug_logError("Data mismatch. \n");
            flash_diag.wr_rd_err++;
            flash_diag.status = 4;
            goto err;
        }
        else
        {
            FLASH_log("QSPI flash test in INDAC mode at 133MHz RCLK have passed\r\n");
            flash_diag.status = 3;
            flash_diag.wr_rd_oks++;
        }
        start_offset += test_len;
    }
err:
    flash_diag.wr_rd_running = 0;
}

