/**
 *  \file udma_apputils.c
 *
 *  \brief Common UDMA application utility used in all UDMA example.
 *
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */

#include "ti/drv/udma/udma.h"
#include "ti/drv/udma/include/udma_ch.h"
#include "ti/csl/csl_rat.h"
#include "ti/drv/sciclient/sciclient.h"
#include "udma_apputils.h"
#if defined(BUILD_C7X)
#include <ti/csl/csl_clec.h>
#include <ti/csl/arch/csl_arch.h>
#endif


#include "debug_config.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

uint64_t Udma_appVirtToPhyFxn(const void *virtAddr)
{
    uint64_t phyAddr;
    uint64_t atcmSizeLocal = 0U;
    uint64_t atcmBaseGlobal = 0U;

#if defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#if defined(BUILD_MCU2_0)
    uint64_t mainOcmcBaseLocal = MAIN_OCM_VIRT_BASE;
    uint64_t mainOcmcBaseGlobal = CSL_MSRAM_512K0_RAM_BASE;
    uint64_t mainOcmcSize = (512U * 1024U);
#endif
#endif

    phyAddr = (uint64_t)virtAddr;
#if defined(BUILD_C66X)
    /* Convert local L2RAM address to global space */
    if ((phyAddr >= CSL_C66_COREPAC_L2_BASE) &&
        (phyAddr < (CSL_C66_COREPAC_L2_BASE + CSL_C66_COREPAC_L2_SIZE)))
    {
#if defined(BUILD_C66X_1)
        phyAddr -= CSL_C66_COREPAC_L2_BASE;
        phyAddr += CSL_C66SS0_C66_SDMA_L2SRAM_0_BASE;
#endif
#if defined(BUILD_C66X_2)
        phyAddr -= CSL_C66_COREPAC_L2_BASE;
        phyAddr += CSL_C66SS1_C66_SDMA_L2SRAM_0_BASE;
#endif
    }
#endif

    /* Convert local MCU domain R5 TCMA address to global space */
#if defined(SOC_AM65XX)
    atcmSizeLocal = CSL_MCU_ATCM_SIZE;
#if defined(BUILD_MCU1_0)
    atcmBaseGlobal = CSL_MCU_ARMSS0_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU1_1)
    atcmBaseGlobal = CSL_MCU_ARMSS0_CORE1_ATCM_BASE;
#endif
#endif

#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#if defined(BUILD_MCU1_0)
#if defined(SOC_J721E)
    atcmSizeLocal = CSL_MCU_ARMSS_ATCM_SIZE;
#else
    atcmSizeLocal = CSL_MCU_R5FSS0_ATCM_SIZE;
#endif
    atcmBaseGlobal = CSL_MCU_R5FSS0_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU1_1)
#if defined(SOC_J721E)
    atcmSizeLocal = CSL_MCU_ARMSS_ATCM_SIZE;
#else
    atcmSizeLocal = CSL_MCU_R5FSS0_ATCM_SIZE;
#endif
    atcmBaseGlobal = CSL_MCU_R5FSS0_CORE1_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_0)
#if defined(SOC_J721E)
    atcmSizeLocal = CSL_ARMSS_ATCM_BASE;
#else
    atcmSizeLocal = CSL_R5FSS0_ATCM_SIZE;
#endif
    atcmBaseGlobal = CSL_R5FSS0_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_1)
#if defined(SOC_J721E)
    atcmSizeLocal = CSL_ARMSS_ATCM_BASE;
#else
    atcmSizeLocal = CSL_R5FSS0_ATCM_SIZE;
#endif
    atcmBaseGlobal = CSL_R5FSS0_CORE1_ATCM_BASE;
#endif
#if defined(BUILD_MCU3_0)
#if defined(SOC_J721E)
    atcmSizeLocal = CSL_ARMSS_ATCM_BASE;
#else
    atcmSizeLocal = CSL_R5FSS0_ATCM_SIZE;
#endif
    atcmBaseGlobal = CSL_R5FSS1_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU3_1)
#if defined(SOC_J721E)
    atcmSizeLocal = CSL_ARMSS_ATCM_BASE;
#else
    atcmSizeLocal = CSL_R5FSS0_ATCM_SIZE;
#endif
    atcmBaseGlobal = CSL_R5FSS1_CORE1_ATCM_BASE;
#endif
#endif

#if defined(SOC_AM64X)
#if defined(BUILD_MCU1_0)
    atcmSizeLocal = CSL_R5FSS0_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS0_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU1_1)
    atcmSizeLocal = CSL_R5FSS0_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS0_CORE1_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_0)
    atcmSizeLocal = CSL_R5FSS1_ATCM_BASE;
    atcmBaseGlobal = CSL_R5FSS1_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_1)
    atcmSizeLocal = CSL_R5FSS1_ATCM_BASE;
    atcmBaseGlobal = CSL_R5FSS1_CORE1_ATCM_BASE;
#endif
#endif

    /* check for start address avoided since atcmBase is 0U */
    if (phyAddr < atcmSizeLocal)
    {
        phyAddr += atcmBaseGlobal;
    }

#if defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#if defined(BUILD_MCU2_0)
    if ((phyAddr >= mainOcmcBaseLocal) && (phyAddr < mainOcmcBaseLocal + mainOcmcSize))
    {
        phyAddr -= mainOcmcBaseLocal;
        phyAddr += mainOcmcBaseGlobal;
    }
#endif
#endif

    return (phyAddr);
}

void *Udma_appPhyToVirtFxn(uint64_t phyAddr)
{
    void *virtAddr;

#if defined(__aarch64__) || defined(BUILD_C7X)
    virtAddr = (void *)phyAddr;
#else
    uint32_t temp;
    uint64_t atcmBaseGlobal = 0U;
    uint64_t atcmSizeGlobal = 0U;

#if defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#if defined(BUILD_MCU2_0)
    uint64_t mainOcmcBaseLocal = MAIN_OCM_VIRT_BASE;
    uint64_t mainOcmcBaseGlobal = CSL_MSRAM_512K0_RAM_BASE;
    uint64_t mainOcmcSize = (512U * 1024U);
#endif
#endif

    /* Convert global L2RAM address to local space */
#if defined(BUILD_C66X_1)
    if ((phyAddr >= CSL_C66SS0_C66_SDMA_L2SRAM_0_BASE) &&
        (phyAddr < (CSL_C66SS0_C66_SDMA_L2SRAM_0_BASE + CSL_C66_COREPAC_L2_SIZE)))
    {
        phyAddr -= CSL_C66SS0_C66_SDMA_L2SRAM_0_BASE;
        phyAddr += CSL_C66_COREPAC_L2_BASE;
    }
#endif
#if defined(BUILD_C66X_2)
    if ((phyAddr >= CSL_C66SS1_C66_SDMA_L2SRAM_0_BASE) &&
        (phyAddr < (CSL_C66SS1_C66_SDMA_L2SRAM_0_BASE + CSL_C66_COREPAC_L2_SIZE)))
    {
        phyAddr -= CSL_C66SS1_C66_SDMA_L2SRAM_0_BASE;
        phyAddr += CSL_C66_COREPAC_L2_BASE;
    }
#endif

/* Convert global TCMA address to local space */
#if defined(SOC_AM65XX)
#if defined(BUILD_MCU1_0)
    atcmBaseGlobal = CSL_MCU_ARMSS0_CORE0_ATCM_BASE;
    atcmSizeGlobal = CSL_MCU_ARMSS0_CORE0_ATCM_SIZE;
#endif
#if defined(BUILD_MCU1_1)
    atcmBaseGlobal = CSL_MCU_ARMSS0_CORE1_ATCM_BASE;
    atcmSizeGlobal = CSL_MCU_ARMSS0_CORE1_ATCM_SIZE;
#endif
#endif

#if defined(SOC_J721E) || defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#if defined(BUILD_MCU1_0)
    atcmSizeGlobal = CSL_MCU_R5FSS0_CORE0_ATCM_SIZE;
    atcmBaseGlobal = CSL_MCU_R5FSS0_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU1_1)
    atcmSizeGlobal = CSL_MCU_R5FSS0_CORE1_ATCM_SIZE;
    atcmBaseGlobal = CSL_MCU_R5FSS0_CORE1_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_0)
    atcmSizeGlobal = CSL_R5FSS0_CORE0_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS0_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_1)
    atcmSizeGlobal = CSL_R5FSS0_CORE1_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS0_CORE1_ATCM_BASE;
#endif
#if defined(BUILD_MCU3_0)
    atcmSizeGlobal = CSL_R5FSS1_CORE0_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS1_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU3_1)
    atcmSizeGlobal = CSL_R5FSS1_CORE1_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS1_CORE1_ATCM_BASE;
#endif
#endif

#if defined(SOC_AM64X)
#if defined(BUILD_MCU1_0)
    atcmSizeGlobal = CSL_R5FSS0_CORE0_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS0_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU1_1)
    atcmSizeGlobal = CSL_R5FSS0_CORE1_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS0_CORE1_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_0)
    atcmSizeGlobal = CSL_R5FSS1_CORE0_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS1_CORE0_ATCM_BASE;
#endif
#if defined(BUILD_MCU2_1)
    atcmSizeGlobal = CSL_R5FSS1_CORE1_ATCM_SIZE;
    atcmBaseGlobal = CSL_R5FSS1_CORE1_ATCM_BASE;
#endif
#endif

    if ((phyAddr >= atcmBaseGlobal) &&
        (phyAddr < (atcmBaseGlobal + atcmSizeGlobal)))
    {
        phyAddr -= atcmBaseGlobal;
        /*start address not added since atcmBase is 0U */
    }

    /* R5/C66x is 32-bit; need to truncate to avoid void * typecast error */
    temp = (uint32_t)phyAddr;
    virtAddr = (void *)temp;

#if defined(SOC_J7200) || defined(SOC_J721S2) || defined(SOC_J784S4)
#if defined(BUILD_MCU2_0)
    if ((phyAddr >= mainOcmcBaseGlobal) && (phyAddr < mainOcmcBaseGlobal + mainOcmcSize))
    {
        phyAddr -= mainOcmcBaseGlobal;
        phyAddr += mainOcmcBaseLocal;
    }
    temp = (uint32_t)phyAddr;
    virtAddr = (void *)temp;
#endif
#endif
#endif

    return (virtAddr);
}


/* Memory pool handle */
struct Udma_DrvObj gUdmaDrvObj;

/**
 * @brief Initialize UDMA library
 * @return UDMA_SOK if success, else error code
 */

int32_t UDMA_Lib_Init()
{
    uint32_t instId;
    int32_t retVal;
    Udma_InitPrms initPrms;

    instId = UDMA_INST_ID_MAIN_0;

#if defined UDMA_OVERRIDE_DEF_RM_SHARED_RES_PRMS
    /* Override the default RM Shared Resource parameters.
     *  For example, using maximum no.of available resources of Global Events
     *  for current instance and updating mimium requirement to 10U */
    Udma_RmSharedResPrms *rmSharedResPrms;

    rmSharedResPrms = Udma_rmGetSharedResPrms(UDMA_RM_RES_ID_GLOBAL_EVENT);
    if (NULL_PTR != rmSharedResPrms)
    {
        rmSharedResPrms->instShare[instId] = UDMA_RM_SHARED_RES_CNT_REST;
        rmSharedResPrms->minReq = 10U;
        /* #UdmaInitPrms_init makes use of this information,
         * in initilaizing #Udma_RmInitPrms */
    }
    else
    {
        DMA_log("Global Event is not a shared resource!!\n");
    }
#endif

    /* UDMA driver init */
    retVal = UdmaInitPrms_init(instId, &initPrms);
    if (UDMA_SOK != retVal)
    {
        DMA_log("[Error] UDMA init prms init failed!!\n");
    }
    else
    {
        initPrms.virtToPhyFxn = &Udma_appVirtToPhyFxn;
        initPrms.phyToVirtFxn = &Udma_appPhyToVirtFxn;
        retVal = Udma_init(&gUdmaDrvObj, &initPrms);  
        if (UDMA_SOK != retVal)
        {
            DMA_log("[Error] UDMA init failed!!\n");
        }
    }
    return retVal;
}


/**
 * @brief Build Host Descriptor
 * @param chHandle UDMA channel handle
 * @param pHpd pointer to Host Packet Descriptor
 * @param bufPtr pointer to buffer
 * @param length length of buffer
 */
void UDMA_Hpd_Init(Udma_ChHandle chHandle,
                   CSL_UdmapCppi5HMPD *pHpd,
                   uint64_t bufPtr,
                   uint32_t length)
{
    uint32_t descType  = (uint32_t)CSL_UDMAP_CPPI5_PD_DESCINFO_DTYPE_VAL_HOST;
    uint32_t cqRingNum = Udma_chGetCqRingNum(chHandle);

    /* Setup descriptor */
    CSL_udmapCppi5SetDescType(pHpd, descType);
    CSL_udmapCppi5SetEpiDataPresent(pHpd, (bool)false);
    CSL_udmapCppi5SetPsDataLoc(pHpd, 0U);
    CSL_udmapCppi5SetPsDataLen(pHpd, 0U);
    CSL_udmapCppi5SetPktLen(pHpd, descType, length);
    CSL_udmapCppi5SetPsFlags(pHpd, 0U);
    CSL_udmapCppi5SetIds(pHpd, descType, 0x321, 0x3FFFU);
    CSL_udmapCppi5SetSrcTag(pHpd, 0x0000); /* Not used */
    CSL_udmapCppi5SetDstTag(pHpd, 0x0000); /* Not used */
    CSL_udmapCppi5SetReturnPolicy(pHpd, descType,
                                  CSL_UDMAP_CPPI5_PD_PKTINFO2_RETPOLICY_VAL_ENTIRE_PKT,
                                  CSL_UDMAP_CPPI5_PD_PKTINFO2_EARLYRET_VAL_NO,
                                  CSL_UDMAP_CPPI5_PD_PKTINFO2_RETPUSHPOLICY_VAL_TO_TAIL,
                                  cqRingNum);
    CSL_udmapCppi5LinkDesc(pHpd, 0U);
    CSL_udmapCppi5SetBufferAddr(pHpd, bufPtr);
    CSL_udmapCppi5SetBufferLen(pHpd, length);
    CSL_udmapCppi5SetOrgBufferAddr(pHpd, bufPtr);
    CSL_udmapCppi5SetOrgBufferLen(pHpd, length);

    CacheP_wbInv((const void *)pHpd, (int32_t)(sizeof(CSL_UdmapCppi5HMPD)));
}
