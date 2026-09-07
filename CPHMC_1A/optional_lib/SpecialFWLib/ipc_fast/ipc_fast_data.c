/**
 *************************************************************************
 * @file      ipc_fast_data.c
 * @author    zht
 * @date      2023/11/08
 * @brief     ipc fast data, for control and protection data
 * @attention None
 *************************************************************************
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "ipc_fast_data.h"

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

#define IPC_FAST_DATA_CORES_NUM   (8)     /* Number of CPUs that are enabled for IPC fast data */
#define IPC_FAST_DATA_BLK_SIZE    (32)    /* a block size in bytes */
#define IPC_FAST_DATA_MAX_SIZE    (0x1000) /* Max size in bytes */
#define IPC_FAST_DATA_MAX_BLK_NUM (IPC_FAST_DATA_MAX_SIZE/IPC_FAST_DATA_BLK_SIZE)   /* Max block number  */

// data
uint8_t gIpcFastData_R0[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_R0")));
uint8_t gIpcFastData_R1[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_R1")));
uint8_t gIpcFastData_R2[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_R2")));
uint8_t gIpcFastData_R3[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_R3")));
uint8_t gIpcFastData_C60[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_C60")));
uint8_t gIpcFastData_C61[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_C61")));
uint8_t gIpcFastData_C70[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_C70")));

uint8_t gIpcFastData_MCU_R1[IPC_FAST_DATA_MAX_SIZE]
    __attribute__((aligned(128), section(".ipc_fast_data_MCU_R1")));

static uint8_t *s_pIPC[IPC_FAST_DATA_CORES_NUM] = {
    gIpcFastData_R0,
    gIpcFastData_R1,
    gIpcFastData_R2,
    gIpcFastData_R3,
    gIpcFastData_C60,
    gIpcFastData_C61,
    gIpcFastData_C70,
    gIpcFastData_MCU_R1,
};


#if defined(BUILD_MCU2_0)
static uint8_t *s_pCurIPC = gIpcFastData_R0;

#elif defined(BUILD_MCU2_1)
static uint8_t *s_pCurIPC = gIpcFastData_R1;

#elif defined(BUILD_MCU3_0)
static uint8_t *s_pCurIPC = gIpcFastData_R2;

#elif defined(BUILD_MCU3_1)
static uint8_t *s_pCurIPC = gIpcFastData_R3;

#elif defined(BUILD_MCU1_1)
static uint8_t *s_pCurIPC = gIpcFastData_MCU_R1;

#elif defined(BUILD_C66X_1)
#include "udma_mem_copy.h"
uint8_t gIpcFastData[IPC_FAST_DATA_MAX_SIZE] __attribute__((aligned(128)));
uint8_t gIpcFastDataOther[4][IPC_FAST_DATA_MAX_SIZE] __attribute__((aligned(128)));

static uint8_t *s_pCurIPC = gIpcFastData;
static uint8_t *s_pIPC_c66[IPC_FAST_DATA_CORES_NUM] = {
    gIpcFastData_R0,
    gIpcFastData_R1,
    &gIpcFastDataOther[0][0],
    &gIpcFastDataOther[1][0],
    gIpcFastData,
    &gIpcFastDataOther[2][0],
    &gIpcFastDataOther[3][0],
};

#elif defined(BUILD_C66X_2)
#include "udma_mem_copy.h"
uint8_t gIpcFastData[IPC_FAST_DATA_MAX_SIZE] __attribute__((aligned(128)));
uint8_t gIpcFastDataOther[4][IPC_FAST_DATA_MAX_SIZE] __attribute__((aligned(128)));

static uint8_t *s_pCurIPC = gIpcFastData;
static uint8_t *s_pIPC_c66[IPC_FAST_DATA_CORES_NUM] = {
    gIpcFastData_R0,
    gIpcFastData_R1,
    &gIpcFastDataOther[0][0],
    &gIpcFastDataOther[1][0],
    &gIpcFastDataOther[2][0],
    gIpcFastData,
    &gIpcFastDataOther[3][0],
};

#elif defined(BUILD_C7X_1)
static uint8_t *s_pCurIPC = gIpcFastData_C70;

#endif

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

/*********************************************************************************
 * 函数：ipc_wr_4dw
 * 描述：核间通讯根据blkNr写入4个dword
 * 调用：无
 * 被调用：由可视化算法块调用
 * 输入：blkNr, in1~in4
 * 输出：N/A
 * 返回：0: 失败; 1: 成功
 * 其他：
 **********************************************************************************/
uint8_t ipc_wr_4dw(uint8_t blkNr, uint32_t in1, uint32_t in2, uint32_t in3, uint32_t in4)
{
    uint32_t offset;
    uint32_t *pTemp;

    if (blkNr >= IPC_FAST_DATA_MAX_BLK_NUM)
        return 0;

    offset = blkNr * IPC_FAST_DATA_BLK_SIZE;
    pTemp = (uint32_t *)(s_pCurIPC + offset);

    pTemp[0] = in1;
    pTemp[1] = in2;
    pTemp[2] = in3;
    pTemp[3] = in4;

    CacheP_wb(pTemp, IPC_FAST_DATA_BLK_SIZE);

    return 1;
}

uint8_t ipc_wr_8dw(uint8_t blkNr, uint32_t in1, uint32_t in2, uint32_t in3, uint32_t in4,
                   uint32_t in5, uint32_t in6, uint32_t in7, uint32_t in8)
{
    uint32_t offset;
    uint32_t *pTemp;

    if (blkNr >= IPC_FAST_DATA_MAX_BLK_NUM)
        return 0;

    offset = blkNr * IPC_FAST_DATA_BLK_SIZE;
    pTemp = (uint32_t *)(s_pCurIPC + offset);

    pTemp[0] = in1;
    pTemp[1] = in2;
    pTemp[2] = in3;
    pTemp[3] = in4;
    pTemp[4] = in5;
    pTemp[5] = in6;
    pTemp[6] = in7;
    pTemp[7] = in8;

    CacheP_wb(pTemp, IPC_FAST_DATA_BLK_SIZE);

    return 1;
}


/*********************************************************************************
 * 函数：ipc_wr_8w
 * 描述：核间通讯根据blkNr写入8个word
 * 调用：无
 * 被调用：由可视化算法块调用
 * 输入：blkNr, in1~in8
 * 输出：N/A
 * 返回：0: 失败; 1: 成功
 * 其他：
 **********************************************************************************/
uint8_t ipc_wr_8w(uint8_t blkNr,
                 uint16_t in1,
                 uint16_t in2,
                 uint16_t in3,
                 uint16_t in4,
                 uint16_t in5,
                 uint16_t in6,
                 uint16_t in7,
                 uint16_t in8)
{
    uint32_t offset;
    uint16_t *pTemp;

    if (blkNr >= IPC_FAST_DATA_MAX_BLK_NUM)
        return 0;

    offset = blkNr * IPC_FAST_DATA_BLK_SIZE;
    pTemp = (uint16_t *)(s_pCurIPC + offset);

    pTemp[0] = in1;
    pTemp[1] = in2;
    pTemp[2] = in3;
    pTemp[3] = in4;
    pTemp[4] = in5;
    pTemp[5] = in6;
    pTemp[6] = in7;
    pTemp[7] = in8;

    CacheP_wb(pTemp, IPC_FAST_DATA_BLK_SIZE);

    return 1;
}

/*********************************************************************************
 * 函数：ipc_wr_real
 * 描述：核间通讯根据blkNr写入4个float
 * 调用：无
 * 被调用：由可视化算法块调用
 * 输入：blkNr, in1~in4
 * 输出：N/A
 * 返回：0: 失败; 1: 成功
 * 其他：
 **********************************************************************************/
uint8_t ipc_wr_real(uint8_t blkNr, float in1, float in2, float in3, float in4)
{
    uint32_t offset;
    float *pTemp;

    if (blkNr >= IPC_FAST_DATA_MAX_BLK_NUM)
        return 0;

    offset = blkNr * IPC_FAST_DATA_BLK_SIZE;
    pTemp = (float *)(s_pCurIPC + offset);

    pTemp[0] = in1;
    pTemp[1] = in2;
    pTemp[2] = in3;
    pTemp[3] = in4;

    CacheP_wb(pTemp, IPC_FAST_DATA_BLK_SIZE);

    return 1;
}

/*********************************************************************************
 * 函数：ipc_rd_blk
 * 描述：核间通讯根据coreNr, blkNr获取block的地址
 * 调用：无
 * 被调用：由可视化算法块调用
 * 输入：coreNr, blkNr
 * 输出：N/A
 * 其他：
 **********************************************************************************/
uint8_t *ipc_rd_blk(uint8_t coreNr, uint8_t blkNr)
{
    uint32_t offset;

    if (coreNr >= IPC_FAST_DATA_CORES_NUM || blkNr >= IPC_FAST_DATA_MAX_BLK_NUM)
        return 0;

    offset = blkNr * IPC_FAST_DATA_BLK_SIZE;

#ifndef BUILD_C66X
    CacheP_Inv(s_pIPC[coreNr] + offset, IPC_FAST_DATA_BLK_SIZE);
    return (s_pIPC[coreNr] + offset);

#else
    CacheP_Inv(s_pIPC_c66[coreNr] + offset, IPC_FAST_DATA_BLK_SIZE);
    return (s_pIPC_c66[coreNr] + offset);
#endif
}

void ipc_wr_update()
{
#ifdef BUILD_C66X_1
    udma_memcpy(s_pIPC[4], s_pCurIPC, IPC_FAST_DATA_MAX_SIZE);
#elif defined(BUILD_C66X_2)
    udma_memcpy(s_pIPC[5], s_pCurIPC, IPC_FAST_DATA_MAX_SIZE);
#endif
}

void ipc_rd_update(uint8_t coreNr)
{
#ifdef BUILD_C66X
    if (coreNr >= IPC_FAST_DATA_CORES_NUM)
        return;
    udma_memcpy(s_pIPC_c66[coreNr], s_pIPC[coreNr], IPC_FAST_DATA_MAX_SIZE);
#endif
}

