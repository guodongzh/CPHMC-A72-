#include "c66x_ecc_test.h"
#ifdef BUILD_C66X
/* ----------------- Global variables ----------------- */
volatile uint32_t gInterruptRecd = 0U;

/* --------------------------------------------------- */
#pragma FUNC_CANNOT_INLINE(dummyAdd)
#pragma CODE_SECTION(dummyAdd, ".L1PSRAM_ADD")

/* Dummy Add function */
int32_t dummyAdd(int32_t a, int32_t b) { return a + b; }
#pragma FUNC_CANNOT_INLINE(dummySub);
#pragma CODE_SECTION(dummySub, ".L1PSRAM_SUB")

/* Dummy Subtract function */
int32_t dummySub(int32_t a, int32_t b) { return a - b; }

/* Interrupt handler for DSP ECC L1 PDMA error
 * Sets global flag based on error info
 */
static void dspEccL1PDmaErrHandler(void *handle)
{
    int32_t retVal = STW_SOK;
    eccDspErrStatus_t errStatus = {0U};
    eccDspErrInfo_t eccDspInfo = {0U};

    retVal = ECCDspIntrGetErrStatus(&errStatus);
    if ((STW_SOK == retVal) && (1U == errStatus.l1PDmaAccessErr))
    {
        ECCDspGetErrInfo(ECC_DSP_MEM_TYPE_L1P, &eccDspInfo);
        if (eccDspInfo.errRamOrCache == 1U)
        {
            printf_("\r\nDSP L1P DMA Error @ address = 0x%x", eccDspInfo.errAddress);

            ECCDspIntrClrErrStatus(&errStatus);
            gInterruptRecd = 1U;
        }
        else
        {
            gInterruptRecd = 0U;
        }
    }
    gInterruptRecd = 1U;
}

/* Function performs DSP ECC L1P DMA test by injection of error through
 * DMA
 */
int32_t dspEccL1PDmaTest(void)
{
    int32_t retVal = STW_SOK;
    uint32_t i = 0U;
    gInterruptRecd = 0U;
    printf_("\r\nDSP ECC L1P DMA Access Error Test", -1);
    /* Initialize the interrupt control */
    Intc_Init();

    /* Enable the interrupt */
    Intc_IntEnable(0);

    /* Registering TimerIsr */
    Intc_IntRegister((uint16_t)DSP_IRQ_113, (IntrFuncPtr)dspEccL1PDmaErrHandler, NULL);

    /* Set the priority */
    Intc_IntPrioritySet((uint16_t)DSP_IRQ_113, (uint16_t)1, (uint8_t)0);

    /* Enable the system interrupt */
    Intc_SystemEnable((uint16_t)DSP_IRQ_113);

    /* Configure SRAM of L1P. */
    DSPICFGCacheEnable(SOC_DSP_ICFG_BASE, DSPICFG_MEM_L1P, DSPICFG_CACHE_SIZE_L1_DISABLED);
    /* Fill the initialization pattern to some location in L2. This will be
     * copied to the targetted location after first enabling ECC.
     */
    // 在 L2 里准备一份128Byte的正常数据
    for (i = 0U; i < (DSP_ECC_NUM_BYTES / 4U); i++)
    {
        HW_WR_REG32(SOC_DSP_L2_BASE + 0x8000U + (4U * i), 0xA1A1A1A1U);
    }
    /* Fill the corruption pattern in some location in L2. This will be used
     * to test the L1P ED for DMA access.
     */
    // 在 L2 里准备一份128Byte的错误数据
    for (i = 0U; i < (DSP_ECC_NUM_BYTES / 4U); i++)
    {
        if (16U == i)
        {
            /* Corrupt One Bit of the 17th word */
            HW_WR_REG32(SOC_DSP_L2_BASE + 0x8100U + (4U * i), 0xA5A1A1A1U);
        }
        else
        {
            HW_WR_REG32(SOC_DSP_L2_BASE + 0x8100U + (4U * i), 0xA1A1A1A1U);
        }
    }

    /* Enable ECC for L1P */
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_ENABLE, 1000U);
    /* Initialize memory using IDMA. Copy the contents from L2 to L1P. */
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_SOURCE, SOC_DSP_L2_BASE + 0x8000U);
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_DEST, SOC_DSP_L1P_BASE);
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_COUNT, DSP_ECC_NUM_BYTES);
    while (HW_RD_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_STAT) & 0x1U != 0U)
    {
        ;
    }

    /* Suspend the ECC operation to corrupt L1P SRAM */
    ECCDspSuspend(ECC_DSP_MEM_TYPE_L1P, 1000U);

    /* Corrupt memory using IDMA. Copy the corrupted array from L2 to L1P */
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_SOURCE, SOC_DSP_L2_BASE + 0x8100U);
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_DEST, SOC_DSP_L1P_BASE);
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_COUNT, DSP_ECC_NUM_BYTES);
    while (HW_RD_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_STAT) & 0x1U != 0U)
    {
        ;
    }

    /* Re-enable the ECC operation */
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_ENABLE, 1000U);
    /* Read the corrupted data to cause a ECC error interrupt.
     * IDMA read from L1P to L2.
     */
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_SOURCE, SOC_DSP_L1P_BASE);
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_DEST, SOC_DSP_L2_BASE + 0x8200U);
    HW_WR_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_COUNT, DSP_ECC_NUM_BYTES);
    while (HW_RD_REG32(SOC_DSP_ICFG_BASE + DSP_IDMA1_STAT) & 0x1U != 0U)
    {
        ;
    }

    /* Wait for interrupt to be recieved */
    while (gInterruptRecd != 1U)
    {
        ;
    }

    /* Disable ECC */
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_DISABLE, 1000U);
    /* Reconfigure L1P cache to the default value of 32K */
    DSPICFGCacheEnable(SOC_DSP_ICFG_BASE, DSPICFG_MEM_L1P, DSPICFG_CACHE_SIZE_L1_32K);
    return retVal;
}

/* Function performs DSP ECC L1P Cache test by injection of error in cache
 */
int32_t dspEccL1PCacheTest(void)
{
    int32_t retVal = STW_SOK;
    volatile int32_t dummyTotal = 0U;
    eccDspErrStatus_t errStatus = {0U};
    eccDspErrInfo_t eccDspInfo = {0U};

    printf_("\r\nDSP ECC L1P Cache Access Error Test");

    /* Configure Cache size of L1P as 4K. */
    DSPICFGCacheEnable(SOC_DSP_ICFG_BASE, DSPICFG_MEM_L1P, DSPICFG_CACHE_SIZE_L1_4K);
    /* Enable ECC for L1P */
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_ENABLE, 1000U);
    /* Initializing the cache line with the code. This function is placed
     * such that the corrupting function call falls exactly in the same
     * L1P cache line. Dummy add is placed in my_l1p_code_sec1 which is in
     * 0x80000400 which will fill a particular L1P cache line. Since the
     * addition operation is called the op code which will fill the cache line
     * is add.
     */
    dummyTotal = dummyAdd(3, 4);
    /* Suspend the ECC operation to corrupt the cache line */
    ECCDspSuspend(ECC_DSP_MEM_TYPE_L1P, 1000U);
    /* Corrupting the Cache line. Since the operation is sub, this will change
     * one bit of the cache line which up till now contained add. The same
     * cache line being corrupted is maintained by placing dummySub at
     * 0x81000400. Since L1P is direct mapped this will replace dummyAdd.
     */
    // 根据注释，dummyAdd 和 dummySub 被链接脚本放在特定地址：
    // dummyAdd 放在 0x80000400
    // dummySub 放在 0x81000400
    // 由于 L1P Cache 是 direct mapped，两个地址会映射到同一个 L1P cache line。
    // 所以执行 dummySub() 时，会把原来 cache line 里的 dummyAdd 指令替换成 dummySub 指令。
    dummyTotal = dummySub(3, 4);
    /* Re-enable the ECC operation */
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_ENABLE, 1000U);
    /* Access the same corrupted cache line to cause a ECC L1P error */
    dummyTotal = dummySub(3, 4);// 触发错误

    ECCDspIntrGetErrStatus(&errStatus);

    if (errStatus.l1PProgramFetchErr == 1U)
    {
        ECCDspGetErrInfo(ECC_DSP_MEM_TYPE_L1P, &eccDspInfo);
        if (eccDspInfo.errRamOrCache == 0U)
        {
            retVal = STW_SOK;

            printf_("\r\nDSP L1P Cache Error @ address = 0x%x", eccDspInfo.errAddress);

            ECCDspIntrClrErrStatus(&errStatus);
            DSPICFGCacheInvalidateAll(SOC_DSP_ICFG_BASE, DSPICFG_MEM_L1P);
        }
        else
        {
            retVal = STW_EFAIL;
        }
    }
    else
    {
        retVal = STW_EFAIL;
    }

    /* Disable ECC */
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_DISABLE, 1000U);
    /* Reconfigure L1P cache to the default value of 32K */
    DSPICFGCacheEnable(SOC_DSP_ICFG_BASE, DSPICFG_MEM_L1P, DSPICFG_CACHE_SIZE_L1_32K);
    ECCDspIntrClrErrStatus(&errStatus);
    return retVal;
}

void L1P_ED_enable(void)
{
    eccDspErrStatus_t errStatus = {0U};

    // 关闭L1P错误检测功能
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_DISABLE, 1000U);

    // 重新配置L1P为32K的cache
    DSPICFGCacheEnable(SOC_DSP_ICFG_BASE, DSPICFG_MEM_L1P, DSPICFG_CACHE_SIZE_L1_32K);

    // 清除DSPICFG_MEM_L1P指定的所有缓存
    DSPICFGCacheInvalidateAll(SOC_DSP_ICFG_BASE, DSPICFG_MEM_L1P);

    // 清除历史EDC错误状态
    ECCDspIntrGetErrStatus(&errStatus);
    ECCDspIntrClrErrStatus(&errStatus);

    // 开启L1P错误检测功能
    ECCDspEnable(ECC_DSP_MEM_TYPE_L1P, ECC_ENABLE, 1000U);
}
#endif