/**
 *  \file boot_app_ospi.c
 *
 *  \brief supporting file for main file for ospi
 */

#include "boot_app_priv.h"
#include "boot_app_ospi.h"
#include "debug_config.h"
#include "flash.h"

/* Offset into app image that is being processed */
Board_flashHandle gOspiHandle;

extern void SBL_DCacheClean(void *addr, uint32_t size);

/**
 * @brief config ospi clk to freq
 * @param freq can be one of flow value
 *          OSPI_MODULE_CLK_133M
 *          OSPI_MODULE_CLK_133M
 *          OSPI_MODULE_CLK_166M
 *          OSPI_MODULE_CLK_160M
 *          OSPI_MODULE_CLK_200M
 */
static void BootApp_OSPI_ConfigClk(uint32_t freq)
{
    OSPI_v0_HwAttrs ospi_cfg;
    int32_t retVal;
    uint64_t ospi_rclk_freq;
    uint32_t parClk;

    uint32_t clkID[] = {
        TISCI_DEV_MCU_FSS0_OSPI_0_OSPI_RCLK_CLK,
        TISCI_DEV_MCU_FSS0_OSPI_1_OSPI_RCLK_CLK};

    uint32_t devID[] = {
        TISCI_DEV_MCU_FSS0_OSPI_0,
        TISCI_DEV_MCU_FSS0_OSPI_1};

    /* Get the default SPI init configurations */
    OSPI_socGetInitCfg(BOARD_OSPI_DOMAIN, BOARD_OSPI_NOR_INSTANCE, &ospi_cfg);

    /* request setting ospi clk */
    retVal = Sciclient_pmModuleClkRequest(devID[BOARD_OSPI_NOR_INSTANCE],
                                          clkID[BOARD_OSPI_NOR_INSTANCE],
                                          TISCI_MSG_VALUE_CLOCK_SW_STATE_REQ,
                                          TISCI_MSG_FLAG_AOP,
                                          SCICLIENT_SERVICE_WAIT_FOREVER);
    if (retVal != CSL_PASS)
    {
        SBL_log(SBL_LOG_NONE, "\n Sciclient_pmModuleClkRequest failed");
        goto clk_cfg_exit;
    }

    /* Max clocks */
    if (freq == OSPI_MODULE_CLK_166M)
    {
        parClk = TISCI_DEV_MCU_FSS0_OSPI_0_OSPI_RCLK_CLK_PARENT_HSDIV4_16FFT_MCU_2_HSDIVOUT4_CLK;

        /* request setting ospi clk */
        retVal = Sciclient_pmSetModuleClkParent(devID[BOARD_OSPI_NOR_INSTANCE],
                                                clkID[BOARD_OSPI_NOR_INSTANCE],
                                                parClk,
                                                SCICLIENT_SERVICE_WAIT_FOREVER);
    }
    else
    {
        parClk = TISCI_DEV_MCU_FSS0_OSPI_0_OSPI_RCLK_CLK_PARENT_HSDIV4_16FFT_MCU_1_HSDIVOUT4_CLK;
        retVal = Sciclient_pmSetModuleClkParent(devID[BOARD_OSPI_NOR_INSTANCE],
                                                clkID[BOARD_OSPI_NOR_INSTANCE],
                                                parClk,
                                                SCICLIENT_SERVICE_WAIT_FOREVER);
    }

    if (retVal != CSL_PASS)
    {
        SBL_log(SBL_LOG_NONE, "\n Sciclient_pmSetModuleClkParent failed");
        goto clk_cfg_exit;
    }

    ospi_cfg.funcClk = freq;
    OSPI_socSetInitCfg(BOARD_OSPI_DOMAIN, BOARD_OSPI_NOR_INSTANCE, &ospi_cfg);

    ospi_rclk_freq = (uint64_t)freq;
    retVal = Sciclient_pmSetModuleClkFreq(devID[BOARD_OSPI_NOR_INSTANCE],
                                          clkID[BOARD_OSPI_NOR_INSTANCE],
                                          ospi_rclk_freq,
                                          TISCI_MSG_FLAG_AOP,
                                          SCICLIENT_SERVICE_WAIT_FOREVER);

    if (retVal != CSL_PASS)
    {
        SBL_log(SBL_LOG_NONE, "\n Sciclient_pmSetModuleClkFreq failed");
        goto clk_cfg_exit;
    }

    ospi_rclk_freq = 0;
    retVal = Sciclient_pmGetModuleClkFreq(devID[BOARD_OSPI_NOR_INSTANCE],
                                          clkID[BOARD_OSPI_NOR_INSTANCE],
                                          &ospi_rclk_freq,
                                          SCICLIENT_SERVICE_WAIT_FOREVER);
    if (retVal != CSL_PASS)
    {
        SBL_log(SBL_LOG_NONE, "\n Sciclient_pmGetModuleClkFreq failed");
        goto clk_cfg_exit;
    }

    SBL_log(SBL_LOG_MAX, "\n OSPI RCLK running at %d MHz. \n", (uint32_t)ospi_rclk_freq);

clk_cfg_exit:
    return;
}

/**
 * @brief initialize ospi flash for boot image
 * @return If successful, return 0. Otherwise, return error code
 */
int32_t BootApp_OSPI_Init()
{
    int32_t retVal;
    retVal = flash_interface_init();
    gOspiHandle = flash_ctrl0_handle;
    return retVal;
}

void BootApp_OSPI_DeInit()
{
    Board_flashClose(gOspiHandle);
}

int32_t BootApp_OSPI_ReadSectors(void *dstAddr, void *srcOffsetAddr, uint32_t length)
{
    uint32_t end_time;
    uint32_t ioMode = OSPI_FLASH_OCTAL_READ;
    uint32_t start_time = CSL_armR5PmuReadCntr(0x1F);

    if (dstAddr == NULL || srcOffsetAddr == NULL)
        return -1;

    Board_flashRead(gOspiHandle, *((uint32_t *)srcOffsetAddr), dstAddr, length, 0);
    SBL_DCacheClean(dstAddr, length);

    end_time = CSL_armR5PmuReadCntr(0x1F);
    for (int i = 0; i < 4; ++i)
    {
        uint8_t *temp = (uint8_t *)dstAddr;
        SBL_log(SBL_LOG_MAX, "dstAddr[%d]=0x%x ", i, temp[i]);
    }
    SBL_log(SBL_LOG_MAX, "\n Ospi Read speed for 0x%x bytes from offset 0x%x = %d Mbytes per sec\n",
            length, *((uint32_t *)srcOffsetAddr), ((400000000 / (end_time - start_time)) * length) / 0x100000);

    *((uint32_t *)srcOffsetAddr) += length;
    return 0;
}

int32_t BootApp_OSPI_EraseSectors(const uint32_t *desOffsetAddr, uint32_t length)
{
    uint32_t sector, page;
    Board_flash_STATUS status;
    Board_FlashInfo *flashInfo;
    flashInfo = (Board_FlashInfo *)gOspiHandle;
    uint32_t addr = *desOffsetAddr;

    if (desOffsetAddr == NULL)
        return -1;

    for (int i = 0; i < length / flashInfo->sector_size; ++i)
    {
        status = Board_flashOffsetToSectorPage(gOspiHandle, addr, &sector, &page);
        if (status != BOARD_FLASH_EOK)
        {
            return status;
        }
        status = Board_flashEraseBlk(gOspiHandle, sector, 0);
        if (status != BOARD_FLASH_EOK)
        {
            return status;
        }
        addr += flashInfo->sector_size;
    }

    return status;
}

int32_t BootApp_OSPI_WriteSectors(void *srcAddr, void *desOffsetAddr, uint32_t length)
{
    if (srcAddr == NULL || desOffsetAddr == NULL)
        return -1;

    Board_flashWrite(gOspiHandle, *((uint32_t *)desOffsetAddr), srcAddr, length, 0);
    return 0;
}

/* move the buffer pointer */
void BootApp_OSPI_SeekMem(void *srcAddr, uint32_t location)
{
    *((uint32_t *)srcAddr) = location;
}

int32_t BootApp_OSPI_ImageLate(sblEntryPoint_t *pEntry, uint32_t imageOffset)
{
    int32_t retVal = E_FAIL;

    uint32_t offset = 0;

    /* Load the MAIN domain remotecore images included in the appimage */
    offset = imageOffset;

    /* used by  SBL_MulticoreImageParse function*/
    fp_readData = &BootApp_OSPI_ReadSectors;
    fp_seek = &BootApp_OSPI_SeekMem;

    retVal = SBL_MulticoreImageParse((void *)&offset, imageOffset, pEntry, SBL_SKIP_BOOT_AFTER_COPY);

    if (retVal != E_PASS)
        UART_printf("Error parsing Main Domain appimage\n");

    return retVal;
}

/**
 * @brief load all image from OSPI flash
 * @param pEntry [out] image entry point
 * @return Error code on file error
 */
int32_t BootApp_OSPI_StageImage(sblEntryPoint_t *pEntry, uint32_t address)
{
    int32_t status = E_FAIL;

    if ((0U != address) && (NULL != pEntry))
    {
        if (address != MAIN_DOMAIN_HLOS)
        {
            status = BootApp_OSPI_ImageLate(pEntry, address);
        }
        else
        {
            /* Load the HLOS appimages */
            status = BootApp_OSPI_ImageLate(pEntry, ATF_SPL_FLASH_ADDR);
            if (status != E_PASS)
            {
                UART_printf("Error parsing A72 appimage #1 for HLOS boot\n");
            }
            else
            {
                /* Set the A72 entry point at the ATF address */
                pEntry->CpuEntryPoint[MPU1_CPU0_ID] = ATF_START_RAM_ADDR;
                BootApp_McuDCacheClean((void *)0x70000000, 0x20000);
            }
        }
    }

    return status;
}
