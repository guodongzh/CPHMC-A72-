/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       tfr_proc.c
 *@author     LiuRui
 *@date       2025.07.23
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.07.23  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "tfr_def.h"
#include <string.h>
#include "ti/osal/CacheP.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
TFR_INF_t g_TfrInf;
uint8_t g_samp_buf[1024] __attribute__((aligned(128))) = {0};
uint32_t g_cur_samp_addr;

#ifndef BUILD_MCU
#include "udma_mem_copy.h"
#endif

// share memory data definition

#if defined(BUILD_MCU2_0)

uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_R0")));
#define TFR_CUR_CORE_ID 0

#elif defined(BUILD_MCU2_1)

uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_R1")));
#define TFR_CUR_CORE_ID 1

#elif defined(BUILD_MCU3_0)

uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_R2")));
#define TFR_CUR_CORE_ID 2

#elif defined(BUILD_MCU3_1)

uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_R3")));
#define TFR_CUR_CORE_ID 3

#elif defined(BUILD_C66X_1)
uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_C60")));
#define TFR_CUR_CORE_ID 4

#elif defined(BUILD_C66X_2)
uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_C61")));
#define TFR_CUR_CORE_ID 5

#elif defined(BUILD_C7X_1)
uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_C71")));
#define TFR_CUR_CORE_ID 6

#elif defined(BUILD_MCU1_1)
uint8_t g_TfrMem[MAX_TFR_SIZE_ONE_CORE] __attribute__((aligned(128), section(".ipc_tfr_mem_MCU1")));
#define TFR_CUR_CORE_ID 7

#endif

#if defined(BUILD_MCU2_0) || defined(BUILD_MCU2_1) || defined(BUILD_MCU3_0) || \
    defined(BUILD_MCU3_1) || defined(BUILD_C66X_1) || defined(BUILD_C66X_2) || \
    defined(BUILD_C7X_1)  || defined(BUILD_MCU1_1)

TFR_INF_t *g_pTfrInfTbl[MAX_TFR_FILE_NUM_ONE_CORE] = {
    (TFR_INF_t *)&g_TfrMem[0],
    (TFR_INF_t *)&g_TfrMem[64]};                       // 128bytes for info
uint8_t *g_pTfrDatBuf = &g_TfrMem[MAX_TFR_INFO_SIZE];  //

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

static __attribute__((always_inline)) int32_t conv_f2i(float val)
{
    return ((val > 0) ? (int32_t)(val + 0.5F) : (int32_t)(val - 0.5F));
}

void tfr_init(uint8_t id,
              uint8_t nbuf,
              uint16_t nAnaCh,
              uint16_t nDigCh,
              uint32_t preFault,
              uint32_t postFault,
              uint32_t samp_period,
              uint8_t *errCode,
              uint32_t *samp_size,
              uint32_t *buf_size,
              uint32_t *tfr_size,
              uint32_t *ana_size,
              uint32_t *dig_size)
{
    uint16_t dig_words, ana_words;
    uint32_t sampSize, sampNum, bufSize, tfrSize, bufOfs;
    TFR_INF_t *pTfrInf;

    *errCode = 0;
    *samp_size = 0;
    *buf_size = 0;
    *tfr_size = 0;

    memset(g_TfrMem, 0, sizeof(g_TfrMem));

    // 16 channel (2 bytes) per group
    if (nDigCh % 16 != 0)
        dig_words = nDigCh / 16 + 1;
    else
        dig_words = nDigCh / 16;

    if (dig_words % 8 != 0)  // 16-byte alignment
        dig_words = (dig_words / 8 + 1) * 8;

    // 4 channel (8 bytes) per group
    if (nAnaCh % 4 != 0)
        ana_words = (nAnaCh / 4 + 1) * 4;
    else
        ana_words = nAnaCh;

    if (ana_words % 8 != 0)  // 16-byte alignment
        ana_words = (ana_words / 8 + 1) * 8;

    sampSize = (ana_words * 2) + (dig_words * 2) + sizeof(SAMP_HDR_t);  // Single package size
    if (sampSize % 8 != 0)                                          // 8-byte alignment
    {
        sampSize = (sampSize / 8 + 1) * 8;
    }
    sampNum = MAX_TFR_DATA_BUF_SIZE / sampSize;  // Total number of packages
    bufSize = sampNum * sampSize;                // The size occupied by all packages

    // check
    if (id >= MAX_TFR_FILE_NUM_ONE_CORE)
    {
        *errCode = 1;
        return;
    }

    if (g_pTfrInfTbl[id]->used)
    {
        *errCode = 2;
        return;
    }

    if (bufSize > MAX_TFR_DATA_BUF_SIZE)
    {
        *errCode = 3;
        return;
    }

    g_pTfrInfTbl[id]->buf_size = bufSize;
    tfrSize = g_pTfrInfTbl[0]->buf_size + g_pTfrInfTbl[1]->buf_size;
    if (tfrSize > MAX_TFR_DATA_BUF_SIZE)
    {
        memset(g_pTfrInfTbl[id], 0, sizeof(TFR_INF_t));
        *errCode = 4;
        return;
    }

    if (id == 1)
    {
        bufOfs = g_pTfrInfTbl[0]->buf_size;
    }
    else
    {
        bufOfs = 0;
    }

    // config info
    pTfrInf = g_pTfrInfTbl[id];
    memset(pTfrInf, 0, sizeof(TFR_INF_t));

    pTfrInf->used = 1;
    pTfrInf->tfr_id = id;
    pTfrInf->cpu_id = TFR_CUR_CORE_ID;
    pTfrInf->buf_num = nbuf;
    pTfrInf->ana_chan_num = nAnaCh;
    pTfrInf->dig_chan_num = nDigCh;
    pTfrInf->pre_samp_num = preFault;
    pTfrInf->pos_samp_num = postFault;
    pTfrInf->samp_period = samp_period;

    // caculation info
    pTfrInf->samp_size = sampSize;
    pTfrInf->samp_num = sampNum;

    pTfrInf->buf_size = bufSize;
    pTfrInf->buf_ofs = bufOfs;

    pTfrInf->ana_size = ana_words * 2;
    pTfrInf->ana_ofs = bufOfs + sizeof(SAMP_HDR_t);

    pTfrInf->dig_size = dig_words * 2;
    pTfrInf->dig_ofs = pTfrInf->ana_ofs + pTfrInf->ana_size;

    pTfrInf->write_pos = 0;
    pTfrInf->cur_samp = (uint32_t )g_pTfrDatBuf[0];
#ifndef BUILD_MCU
    g_cur_samp_addr = pTfrInf->cur_samp;
    memcpy(&g_TfrInf, pTfrInf, sizeof(TFR_INF_t));
    g_TfrInf.cur_samp = (uint32_t)&g_samp_buf[0];
#endif


    *samp_size = sampSize;
    *buf_size = bufSize;
    *tfr_size = tfrSize;

    *ana_size = pTfrInf->ana_size;
    *dig_size = pTfrInf->dig_size;
}

static __attribute__((always_inline)) TFR_INF_t *tfr_get_info(uint8_t id)
{
    if (id >= MAX_TFR_FILE_NUM_ONE_CORE)
        return 0;
#ifdef BUILD_MCU
    return g_pTfrInfTbl[id];
#else
    return &g_TfrInf;
#endif
}

void tfr_update(uint8_t id)
{
    uint32_t datBuf;
    TFR_INF_t *pTfrInf;
    volatile SAMP_HDR_t *pSampHdr;

    static TFR_DATE_t time = {
        .wYear = 2024,
        .byMon = 1,
        .byDay = 10,
        .byHour = 0,
        .byMin = 0,
        .wSec = 0,
        .wMs = 0,
        .wUs = 0,
    };

    pTfrInf = tfr_get_info(id);
    if (pTfrInf == 0)
        return;

    if (pTfrInf->used == 0)
        return;

    // gettime....
//    time.wUs += 100;
//    if (time.wUs == 1000)
//    {
//        time.wUs = 0;
//        time.wMs++;
//        if (time.wMs == 1000)
//        {
//            time.wMs = 0;
//            time.wSec++;
//            if (time.wSec == 60)
//            {
//                time.wSec = 0;
//                time.byMin++;
//                if (time.byMin == 60)
//                {
//                    time.byMin = 0;
//                    time.byHour++;
//                    if (time.byHour == 24)
//                    {
//                        time.byHour = 0;
//                    }
//                }
//            }
//        }
//    }

    APP_UTC_TIME 	UtcTime_src = {0};
    APP_StruTime 	StruTime_dest = {0};

#ifndef BUILD_C66X
    cache_inv_com(&g_shm_irigb_info.Clk_time_Edge, sizeof(CLK_TIME_EDGE), CacheP_TYPE_ALL);
    UtcTime_src.utc_secs=g_shm_irigb_info.Clk_time_Edge.nUTC;
    UtcTime_src.fraction=g_shm_irigb_info.Clk_time_Edge.nFrc;
    UtcTime_src.uSec_100 = g_shm_irigb_info.Clk_time_Edge.uSec_100_cnt;
    UtcTime_To_StruTime_app(&UtcTime_src,&StruTime_dest, g_shm_irigb_info.Clk_time_Edge.sTimeZone_min);
#else
    cache_inv_com(&g_shm_irigb_info_c6x.Clk_time_Edge, sizeof(CLK_TIME_EDGE), CacheP_TYPE_ALL);
    UtcTime_src.utc_secs=g_shm_irigb_info_c6x.Clk_time_Edge.nUTC;
    UtcTime_src.fraction=g_shm_irigb_info_c6x.Clk_time_Edge.nFrc;
    UtcTime_src.uSec_100 = g_shm_irigb_info_c6x.Clk_time_Edge.uSec_100_cnt;
    UtcTime_To_StruTime_app(&UtcTime_src,&StruTime_dest, g_shm_irigb_info_c6x.Clk_time_Edge.sTimeZone_min);
#endif
    time.wYear = StruTime_dest.Year;
    time.byMon = StruTime_dest.Month;
    time.byDay = StruTime_dest.Date;
    time.byHour = StruTime_dest.Hour;
    time.byMin = StruTime_dest.Min;
    time.wSec = StruTime_dest.Sec;
    time.wMs  = StruTime_dest.uSec_100 * 0.1;
    time.wUs  = StruTime_dest.uSec_100 * 100;

    pSampHdr = (SAMP_HDR_t *)pTfrInf->cur_samp;
    if (pSampHdr)
    {
        pSampHdr->year = time.wYear;
        pSampHdr->month = time.byMon;
        pSampHdr->day = time.byDay;
        pSampHdr->hour = time.byHour;
        pSampHdr->min = time.byMin;
        pSampHdr->sec = time.wSec;
        pSampHdr->us = time.wUs + time.wMs * 1000;
    }

    // write back samp
    CacheP_wb((void *)pTfrInf->cur_samp, pTfrInf->samp_size);
#ifdef BUILD_MCU

    // update write_pos, cur_samp addr
    pTfrInf->write_pos++;
    if (pTfrInf->write_pos >= pTfrInf->samp_num)
        pTfrInf->write_pos = 0;

    datBuf = (uint32_t)g_pTfrDatBuf + pTfrInf->buf_ofs;
    pTfrInf->cur_samp = datBuf + pTfrInf->write_pos * pTfrInf->samp_size;

    // write back tfr info
    CacheP_wb(pTfrInf, 128);
#else

    datBuf = (uint32_t)g_pTfrDatBuf + pTfrInf->buf_ofs;
    g_cur_samp_addr = datBuf + pTfrInf->write_pos * pTfrInf->samp_size;
    udma_memcpy((uint8_t *)g_cur_samp_addr, (uint8_t *)pTfrInf->cur_samp, pTfrInf->samp_size);
//     memcpy((void *)g_cur_samp, (void *)pTfrInf->cur_samp, pTfrInf->samp_size);
//     CacheP_wb((void *)g_cur_samp, pTfrInf->samp_size);

    // update write_pos, cur_samp addr
    pTfrInf->write_pos++;
    if (pTfrInf->write_pos >= pTfrInf->samp_num)
        pTfrInf->write_pos = 0;

    pTfrInf->cur_samp = g_cur_samp_addr;
    // write back tfr info
    CacheP_wb(pTfrInf, 128);

    udma_memcpy((uint8_t *)g_pTfrInfTbl[0], (uint8_t *)pTfrInf, 128);
//     memcpy(g_pTfrInfTbl[0], pTfrInf, 128);
//     CacheP_wb(g_pTfrInfTbl[0], 128);
    pTfrInf->cur_samp = (uint32_t)&g_samp_buf[0];
#endif
}

void tfr_trig(uint8_t id, uint8_t trig)
{
    TFR_INF_t *pTfrInf;
    volatile SAMP_HDR_t *pSampHdr;

    pTfrInf = tfr_get_info(id);
    if (pTfrInf == 0)
        return;

    if (pTfrInf->used == 0)
        return;

    pSampHdr = (SAMP_HDR_t *)pTfrInf->cur_samp;
    if (pSampHdr)
    {
        pSampHdr->trig = trig;
    }
}

void tfr_ana_4r(uint8_t id,
                uint8_t blkNr,
                float k1,
                float v1,
                float k2,
                float v2,
                float k3,
                float v3,
                float k4,
                float v4)
{
    volatile int16_t *pAna;
    TFR_INF_t *pTfrInf;

    pTfrInf = tfr_get_info(id);
    if (pTfrInf == 0)
        return;

    if (pTfrInf->used == 0)
        return;

    if (pTfrInf->cur_samp == 0)
        return;

    if ((blkNr + 1) * 2 * 4 > pTfrInf->ana_size)
        return;

    pAna = (int16_t *)(pTfrInf->cur_samp + pTfrInf->ana_ofs + blkNr * 2 * 4);

    pAna[0] = (int16_t)conv_f2i(k1 * v1);
    pAna[1] = (int16_t)conv_f2i(k2 * v2);
    pAna[2] = (int16_t)conv_f2i(k3 * v3);
    pAna[3] = (int16_t)conv_f2i(k4 * v4);
}

void tfr_dig_1w(uint8_t id, uint8_t blkNr, uint16_t v1)
{
    volatile uint16_t *pDig;
    TFR_INF_t *pTfrInf;

    pTfrInf = tfr_get_info(id);
    if (pTfrInf == 0)
        return;

    if (pTfrInf->used == 0)
        return;

    if (pTfrInf->cur_samp == 0)
        return;

    if ((blkNr + 1) * 2 > pTfrInf->dig_size)
        return;

    pDig = (uint16_t *)(pTfrInf->cur_samp + pTfrInf->dig_ofs + blkNr * 2);
    *pDig = v1;
}

void tfr_dig_16b(uint8_t id,
                 uint8_t blkNr,
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
    volatile uint16_t *pDig;
    TFR_INF_t *pTfrInf;

    pTfrInf = tfr_get_info(id);
    if (pTfrInf == 0)
        return;

    if (pTfrInf->used == 0)
        return;

    if (pTfrInf->cur_samp == 0)
        return;

    if ((blkNr + 1) * 2 > pTfrInf->dig_size)
        return;

    pDig = (uint16_t *)(pTfrInf->cur_samp + pTfrInf->dig_ofs + blkNr * 2);
    *pDig = (((uint16_t)(v1 & 0x0001) << 0) | ((uint16_t)(v2 & 0x0001) << 1) |
             ((uint16_t)(v3 & 0x0001) << 2) | ((uint16_t)(v4 & 0x0001) << 3) |
             ((uint16_t)(v5 & 0x0001) << 4) | ((uint16_t)(v6 & 0x0001) << 5) |
             ((uint16_t)(v7 & 0x0001) << 6) | ((uint16_t)(v8 & 0x0001) << 7) |
             ((uint16_t)(v9 & 0x0001) << 8) | ((uint16_t)(v10 & 0x0001) << 9) |
             ((uint16_t)(v11 & 0x0001) << 10) | ((uint16_t)(v12 & 0x0001) << 11) |
             ((uint16_t)(v13 & 0x0001) << 12) | ((uint16_t)(v14 & 0x0001) << 13) |
             ((uint16_t)(v15 & 0x0001) << 14) | ((uint16_t)(v16 & 0x0001) << 15));
}

#endif
