/**
 *************************************************************************
 * @file      ipc_scada_data.c
 * @author    zht
 * @date      2023/11/08
 * @brief     ipc for scada data
 * @attention None
 *************************************************************************
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#ifdef BUILD_MCU

#include "ipc_scada_data.h"
#include "memory_map_defines.h"
#include "ti/osal/CacheP.h"

#define IPC_YX_MAX_NUM (IPC_YX_MAX_BUF_SIZE_WORDS * 16)  // 1024
#define IPC_YK_MAX_NUM (IPC_YK_MAX_BUF_SIZE_WORDS * 8)   // 512
#define IPC_YC_MAX_NUM (IPC_YC_MAX_BUF_SIZE_FLOAT)       // 512
#define IPC_YT_MAX_NUM (IPC_YT_MAX_BUF_SIZE_FLOAT)       // 256
/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

// share memory data definition
IPC_SCADA_CFG_t gIpcScadaCfg __attribute__((aligned(128), section(".ipc_scada_cfg")));                  // 128,
uint16_t gIpcYxData[IPC_YX_MAX_BUF_SIZE_WORDS] __attribute__((aligned(128), section(".ipc_yx_data")));  // 128,  256
uint16_t gIpcYxDataBak[IPC_YX_MAX_BUF_SIZE_WORDS];                                                      // 128,  256
uint16_t gIpcYkData[IPC_YK_MAX_BUF_SIZE_WORDS + 1] __attribute__((aligned(128), section(".ipc_yk_data")));  // 128,  384
float gIpcYcData[IPC_YC_MAX_BUF_SIZE_FLOAT] __attribute__((aligned(128), section(".ipc_yc_data")));     // 2048, 2432
float gIpcYtData[IPC_YT_MAX_BUF_SIZE_FLOAT + 1] __attribute__((aligned(128), section(".ipc_yt_data")));     // 1024, 3456
uint8_t gIpcYtData_flag[(IPC_YT_MAX_BUF_SIZE_FLOAT / 8) + 1] = {0};
/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */

/*********************************************************************************
 * 函数：scada_cfg_init
 * 描述：四遥信息初始化，设置四遥的点个数, 由可视化算法块调用
 * 输入：yxNum, ycNum, ytNum, ykNum;
    其中 yxNum, ykNum: 取16对齐;
         ycNum, ytNum: 取8对齐;
 * 输出：N/A
 * 返回：无
 **********************************************************************************/
void scada_cfg_init(uint16_t yxNum, uint16_t ykNum, uint16_t ycNum, uint16_t ytNum)
{
    void *start = &gIpcScadaCfg;
    memset(start, 0, CORE_4RMT_TOTAL_SIZE);
    memset(gIpcYxDataBak, 0, sizeof(gIpcYxDataBak));
    // yx, 16对齐
    if (yxNum & 0x0F)
    {
        yxNum = (((yxNum >> 4) + 1) << 4);
    }
    if (yxNum > IPC_YX_MAX_NUM)
    {
        yxNum = IPC_YX_MAX_NUM;
    }
    gIpcScadaCfg.yx_num = yxNum;

    // yk
    if (ykNum & 0x0F)
    {
        ykNum = (((ykNum >> 4) + 1) << 4);
    }
    if (ykNum > IPC_YK_MAX_NUM)
    {
        ykNum = IPC_YK_MAX_NUM;
    }
    gIpcScadaCfg.yk_num = ykNum + 1;

    // yc, 8对齐
    if (ycNum & 0x07)
    {
        ycNum = (((ycNum >> 3) + 1) << 3);
    }
    if (ycNum > IPC_YC_MAX_NUM)
    {
        ycNum = IPC_YC_MAX_NUM;
    }
    gIpcScadaCfg.yc_num = ycNum;

    // yt
    if (ytNum & 0x07)
    {
        ytNum = (((ytNum >> 3) + 1) << 3);
    }
    if (ytNum > IPC_YT_MAX_NUM)
    {
        ytNum = IPC_YT_MAX_NUM;
    }
    gIpcScadaCfg.yt_num = ytNum + 1;
    CacheP_wb(start, CORE_4RMT_TOTAL_SIZE);
}

/*********************************************************************************
 * 函数：get_scada_cfg
 * 描述：获取该核的四遥点数, 由可视化算法块调用
 * 输入：无
 * 输出：yxNum, ykNum, ycNum, ytNum
 * 返回：无
 **********************************************************************************/
void get_scada_cfg(uint16_t *yxNum, uint16_t *ykNum, uint16_t *ycNum, uint16_t *ytNum)
{
    *yxNum = gIpcScadaCfg.yx_num;
    *ykNum = gIpcScadaCfg.yk_num - 1;
    *ycNum = gIpcScadaCfg.yc_num;
    *ytNum = gIpcScadaCfg.yt_num - 1;
}

/*********************************************************************************
 * 函数：write_yx_1b
 * 描述：写遥信1个点, 由可视化算法块调用
 * 输入：grp_ofs: 组号,16点一组, 从0开始, bitNo: 位号, v1: 值
 * 输出：无
 * 返回：无
 **********************************************************************************/
void write_yx_1b(uint16_t grp_ofs, uint8_t bitNo, uint8_t v1)
{
    uint16_t yxGrp = gIpcScadaCfg.yx_num >> 4;

    if (grp_ofs >= yxGrp || bitNo >= 16)
    {
        return;
    }

    if (v1 != 0)  // 变1
    {
        gIpcYxData[grp_ofs] |= (0x0001 << bitNo);
    }
    else  // 变0
    {
        gIpcYxData[grp_ofs] &= (uint16_t)(~(0x0001 << bitNo));
    }
    CacheP_wb(&gIpcYxData[grp_ofs], 16u);
}

/*********************************************************************************
 * 函数：write_yx_1w
 * 描述：用1个word写遥信16个点, 由可视化算法块调用
 * 输入：grp_ofs: 组号,16点一组, 从0开始, v1: 值
 * 输出：无
 * 返回：无
 **********************************************************************************/
void write_yx_1w(uint16_t grp_ofs, uint16_t v1)
{
    uint16_t yxGrp = gIpcScadaCfg.yx_num >> 4;

    if (grp_ofs >= yxGrp)
    {
        return;
    }

    gIpcYxData[grp_ofs] = v1;
    CacheP_wb(&gIpcYxData[grp_ofs], 16u);
}

/*********************************************************************************
 * 函数：write_yx_16b
 * 描述：写遥信16个点, 由可视化算法块调用
 * 输入：grp_ofs: 组号,16点一组，从0开始, v1~v16
 * 输出：无
 * 返回：无
 **********************************************************************************/
void write_yx_16b(uint16_t grp_ofs,
                  uint8_t v1,
                  uint8_t v2,
                  uint8_t v3,
                  uint8_t v4,
                  uint8_t v5,
                  uint8_t v6,
                  uint8_t v7,
                  uint8_t v8,
                  uint8_t v9,
                  uint8_t v10,
                  uint8_t v11,
                  uint8_t v12,
                  uint8_t v13,
                  uint8_t v14,
                  uint8_t v15,
                  uint8_t v16)
{
    uint16_t yxGrp = gIpcScadaCfg.yx_num >> 4;

    if (grp_ofs >= yxGrp)
    {
        return;
    }

    gIpcYxData[grp_ofs] =
        (((uint16_t)(v1 & 0x0001) << 0) | ((uint16_t)(v2 & 0x0001) << 1) | ((uint16_t)(v3 & 0x0001) << 2) |
         ((uint16_t)(v4 & 0x0001) << 3) | ((uint16_t)(v5 & 0x0001) << 4) | ((uint16_t)(v6 & 0x0001) << 5) |
         ((uint16_t)(v7 & 0x0001) << 6) | ((uint16_t)(v8 & 0x0001) << 7) | ((uint16_t)(v9 & 0x0001) << 8) |
         ((uint16_t)(v10 & 0x0001) << 9) | ((uint16_t)(v11 & 0x0001) << 10) | ((uint16_t)(v12 & 0x0001) << 11) |
         ((uint16_t)(v13 & 0x0001) << 12) | ((uint16_t)(v14 & 0x0001) << 13) | ((uint16_t)(v15 & 0x0001) << 14) |
         ((uint16_t)(v16 & 0x0001) << 15));
    CacheP_wb(&gIpcYxData[grp_ofs], 16u);
}

/*********************************************************************************
 * 函数：write_yc_1r
 * 描述：写遥测1个点, 由可视化算法块调用
 * 输入：ofs：点号, 从0开始, v1：值
 * 输出：无
 * 返回：无
 **********************************************************************************/
void write_yc_1r(uint16_t ofs, float v1)
{
    if (ofs < gIpcScadaCfg.yc_num)
    {
        gIpcYcData[ofs] = v1;
        CacheP_wb(&gIpcYcData[ofs], 4u);
    }
}

/*********************************************************************************
 * 函数：write_yc_3r
 * 描述：写遥测3个点, 由可视化算法块调用
 * 输入：ofs：点号, 从0开始, v1：值1, v2：值2, v3：值3
 * 输出：无
 * 返回：无
 **********************************************************************************/
void write_yc_3r(uint16_t ofs, float v1, float v2, float v3)
{
    if (ofs + 2 < gIpcScadaCfg.yc_num)
    {
        gIpcYcData[ofs] = v1;
        gIpcYcData[ofs + 1] = v2;
        gIpcYcData[ofs + 2] = v3;
        CacheP_wb(&gIpcYcData[ofs], 12u);
    }
}

/*********************************************************************************
 * 函数：write_yc_6r
 * 描述：写遥测6个点, 由可视化算法块调用
 * 输入：ofs：点号, 从0开始, v1：值1, v2：值2, v3：值3, v4：值4, v5：值5, v6：值6
 * 输出：无
 * 返回：无
 **********************************************************************************/
void write_yc_6r(uint16_t ofs, float v1, float v2, float v3, float v4, float v5, float v6)
{
    if (ofs + 5 < gIpcScadaCfg.yc_num)
    {
        gIpcYcData[ofs] = v1;
        gIpcYcData[ofs + 1] = v2;
        gIpcYcData[ofs + 2] = v3;
        gIpcYcData[ofs + 3] = v4;
        gIpcYcData[ofs + 4] = v5;
        gIpcYcData[ofs + 5] = v6;
        CacheP_wb(&gIpcYcData[ofs], 24u);
    }
}

/*********************************************************************************
 * 函数：write_yc_8r
 * 描述：写遥测8个点, 由可视化算法块调用
 * 输入：ofs：点号, 从0开始, v1：值1, v2：值2, v3：值3, v4：值4, v5：值5, v6：值6, v7：值7, v8：值8
 * 输出：无
 * 返回：无
 **********************************************************************************/
void write_yc_8r(uint16_t ofs, float v1, float v2, float v3,
                 float v4, float v5, float v6, float v7,
                 float v8)
{
    if (ofs + 7 < gIpcScadaCfg.yc_num)
    {
        gIpcYcData[ofs] = v1;
        gIpcYcData[ofs + 1] = v2;
        gIpcYcData[ofs + 2] = v3;
        gIpcYcData[ofs + 3] = v4;
        gIpcYcData[ofs + 4] = v5;
        gIpcYcData[ofs + 5] = v6;
        gIpcYcData[ofs + 6] = v7;
        gIpcYcData[ofs + 7] = v8;
        CacheP_wb(&gIpcYcData[ofs], 32u);
    }
}

/*********************************************************************************
 * 函数：read_yt_1r
 * 描述：读遥调1个点, 由可视化算法块调用
 * 输入：ofs：点号, 从1开始, v1
 * 输出：v1
 * 返回：无
 **********************************************************************************/
void read_yt_1r(uint16_t ofs, float *v1, uint8_t *isvalid)
{
    if (ofs < gIpcScadaCfg.yt_num)
    {
        *v1 = gIpcYtData[ofs];
        *isvalid = (gIpcYtData_flag[ofs / 8] & (1 << (ofs % 8))) >> (ofs % 8);
    }
}

/*********************************************************************************
 * 函数：read_yt_8r
 * 描述：读遥调8个点, 由可视化算法块调用
 * 输入：ofs：点号, 从1开始, v1~v8
 * 输出：v1~v8
 * 返回：无
 **********************************************************************************/
void read_yt_8r(uint16_t ofs, float *v1, float *v2, float *v3, float *v4, float *v5, float *v6, float *v7, float *v8,
                uint8_t *isvalid1, uint8_t *isvalid2, uint8_t *isvalid3, uint8_t *isvalid4,
                uint8_t *isvalid5, uint8_t *isvalid6, uint8_t *isvalid7, uint8_t *isvalid8)
{
    if (ofs + 7 < gIpcScadaCfg.yt_num)
    {
        *v1 = gIpcYtData[ofs];
        *v2 = gIpcYtData[ofs + 1];
        *v3 = gIpcYtData[ofs + 2];
        *v4 = gIpcYtData[ofs + 3];
        *v5 = gIpcYtData[ofs + 4];
        *v6 = gIpcYtData[ofs + 5];
        *v7 = gIpcYtData[ofs + 6];
        *v8 = gIpcYtData[ofs + 7];
        *isvalid1 = (gIpcYtData_flag[(ofs) / 8]      & (1 << ((ofs) % 8)     )) >> ((ofs) % 8)     ;
        *isvalid2 = (gIpcYtData_flag[(ofs + 1 ) / 8] & (1 << ((ofs + 1 ) % 8))) >> ((ofs + 1 ) % 8);
        *isvalid3 = (gIpcYtData_flag[(ofs + 2 ) / 8] & (1 << ((ofs + 2 ) % 8))) >> ((ofs + 2 ) % 8);
        *isvalid4 = (gIpcYtData_flag[(ofs + 3 ) / 8] & (1 << ((ofs + 3 ) % 8))) >> ((ofs + 3 ) % 8);
        *isvalid5 = (gIpcYtData_flag[(ofs + 4 ) / 8] & (1 << ((ofs + 4 ) % 8))) >> ((ofs + 4 ) % 8);
        *isvalid6 = (gIpcYtData_flag[(ofs + 5 ) / 8] & (1 << ((ofs + 5 ) % 8))) >> ((ofs + 5 ) % 8);
        *isvalid7 = (gIpcYtData_flag[(ofs + 6 ) / 8] & (1 << ((ofs + 6 ) % 8))) >> ((ofs + 6 ) % 8);
        *isvalid8 = (gIpcYtData_flag[(ofs + 7 ) / 8] & (1 << ((ofs + 7 ) % 8))) >> ((ofs + 7 ) % 8);
    }
}

/*********************************************************************************
 * 函数：read_yk
 * 描述：读遥控1个点, 由可视化算法块调用
 * 输入：ofs：点号从1开始, v1：分离指令, v2：闭合指令
 * 输出：无
 * 返回：无
 **********************************************************************************/
void read_yk(uint16_t ofs, uint8_t *open, uint8_t *close)
{
    uint16_t idx;
    uint8_t bitNo;

    if (ofs >= gIpcScadaCfg.yk_num)
    {
        *open = 0;
        *close = 0;
        return;
    }

    idx = ofs / 8;
    bitNo = (ofs % 8) * 2;

    *close = ((gIpcYkData[idx] & (1 << bitNo)) >> bitNo);             // 闭合指令（有效）, 5s脉冲
    *open = ((gIpcYkData[idx] & (1 << (bitNo + 1))) >> (bitNo + 1));  // 分离指令（无效）, 5s脉冲
}

#endif