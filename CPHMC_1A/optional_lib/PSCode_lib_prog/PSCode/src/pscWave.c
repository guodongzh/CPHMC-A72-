/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscWave.c
 *@author     jinyangh
 *@date       2026.02.25
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.02.25  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscWave.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
psc_wave         g_psc_wave_list __attribute__((section(".wave_list")));; // R0写，其他核读
psc_wave_data    g_psc_wave_data[PSC_WAVE_MAX_CHANNEL_NUM] __attribute__((section(".wave_data")));; // 其他核写，R0读
uint8_t          wave_buf[5000]; // 用于暂存录波组包数据
uint8_t          wave_frame_head[WAVE_MESS_HEAD_OFS] = {0x52, 0x58, 0x48, 0x4B};
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/**
 * @brief        : 可视化录波功能初始化
 * @attention    : g_psc_wave_list - R0写,其他核读
 *                 g_psc_wave_data - 其他核写,R0读
 */
void PscWaveInit(void)
{
    memset(&g_psc_wave_list, 0, sizeof(g_psc_wave_list));
    memset(g_psc_wave_data, 0, sizeof(g_psc_wave_data));
    PscCacheWb(&g_psc_wave_list.state, sizeof(g_psc_wave_list.state));
    PscCacheWb(&g_psc_wave_data, sizeof(g_psc_wave_data));
}

/**
 * @brief        : 清除录波列表
 * @attention    : 清除录波列表后，要清除录波数据buffer的当前下标和上一次下标
 *                 因为有可能在两次中断的间隔中，就完成了整个录波列表的更改，所以用于标记的所有下标应该全清0
 *                 不能把pre_idx值改为cur_idx，因为可能刚把pre_idx改为cur_idx后，其他核马上会把cur_idx++，
 *                 而此时pre_idx指向的值是个之前录波数据的值
 */
void PscWaveDiscard(void)
{
    g_psc_wave_list.state                = PSCFALSE;
    g_psc_wave_list.com                  = 0;
    g_psc_wave_list.used_channel_num     = 0;
    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        g_psc_wave_list.channel[i].use_flag = PSCFALSE;
        g_psc_wave_list.channel[i].pre_idx  = 0;
    }
    g_psc_wave_list.cnt[g_pscode_udp_info.pscode_src_port - PORT_ID_BASE]++; // 代表录波列表的下标应该清0
    PscCacheWb(&g_psc_wave_list.state, sizeof(g_psc_wave_list.state));
}

/**
 * @brief             : 添加录波信息
 * @param  pWatchList : 上位机下发的录波信息
 * @attention         : 如果无空闲位置，则丢弃
 */
void PscAddWaveList(tPscDataAdr *pWatchList)
{
    g_psc_wave_list.com = g_pscode_udp_info.pscode_src_port;

    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        if (g_psc_wave_list.channel[i].use_flag == PSCFALSE)
        {
            g_psc_wave_list.channel[i].use_flag = PSCTRUE;
            g_psc_wave_list.channel[i].pgm      = pWatchList->m_Pgm;
            g_psc_wave_list.channel[i].addr     = pWatchList->m_Addr;
            g_psc_wave_list.channel[i].size     = pWatchList->m_Size;
            g_psc_wave_list.used_channel_num++;
            break;
        }
    }
}

/**
 * @brief             : 使能录波功能
 */
void PscWaveEnable(void)
{
    if (g_psc_wave_list.used_channel_num > 0)
    {
        g_psc_wave_list.state = PSCTRUE;
        PscCacheWb(&g_psc_wave_list, sizeof(g_psc_wave_list));
    }
}

/**
 * @brief             : 将录波数据组包发出
 * @attention         : 此逻辑不需要在中断中执行，当Cache inv后，如果中断此时新改了值也没关系，只看刷出来的cur_idx值
 */
void PscDrawWave(void)
{
    uint8_t   *pbuf = &wave_buf[0];
    uint32_t   cur_idx[PSC_WAVE_MAX_CHANNEL_NUM];
    uint32_t   pre_idx[PSC_WAVE_MAX_CHANNEL_NUM];
    uint32_t   diff[PSC_WAVE_MAX_CHANNEL_NUM];
    uint32_t   len = 0;
    static uint64_t cnt = 0;

    if (g_psc_wave_list.state != PSCTRUE)
    {
        return;
    }

    PscCacheInv(g_psc_wave_data, sizeof(g_psc_wave_data));
    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        cur_idx[i] = g_psc_wave_data[i].cur_idx;
        pre_idx[i] = g_psc_wave_list.channel[i].pre_idx;
        diff[i]    = (cur_idx[i] + PSC_WAVE_DATA_NUM_MAX - pre_idx[i]) % PSC_WAVE_DATA_NUM_MAX;
    }

    // header
    memcpy (pbuf + len, wave_frame_head, WAVE_MESS_HEAD_OFS);
    len += WAVE_MESS_HEAD_OFS + WAVE_MESS_LEN_OFS;

    // version
    PscMemAbsSetWord(pbuf + len, 0);
    len += WAVE_MESS_VERSION_OFS;

    // counter
    PscMemAbsSetDword(pbuf + len, cnt & 0xFFFFFFFF);
    PscMemAbsSetDword(pbuf + len + 4, cnt >> 32);
    len += WAVE_MESS_CNT_OFS;
    cnt++;

    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        if (g_psc_wave_list.channel[i].use_flag == PSCFALSE)
        {
            continue;
        }

        uint16_t size     = g_psc_wave_list.channel[i].size;
        uint32_t gap_time = PscGetCoreIsrTime() * diff[i];

        // time
        PscMemAbsSetDword(pbuf + len, gap_time);
        len += WAVE_MESS_TIME_OFS;

        // pgm
        PscMemAbsSetWord(pbuf + len, g_psc_wave_list.channel[i].pgm);
        len += 2;

        // seg
        PscMemAbsSetWord(pbuf + len, g_psc_wave_list.channel[i].addr >> 16);
        len += 2;

        // offset
        PscMemAbsSetWord(pbuf + len, g_psc_wave_list.channel[i].addr & 0xFFFF);
        len += 2;

        //size
        PscMemAbsSetWord(pbuf + len, g_psc_wave_list.channel[i].size);
        len += 2;

        // isr time
        PscMemAbsSetWord(pbuf + len, PscGetCoreIsrTime());
        len += WAVE_MESS_ISR_OFS;

        // data num
        PscMemAbsSetWord(pbuf + len, diff[i]);
        len += WAVE_MESS_DATA_NUM_OFS;

        // data
        for (uint32_t j = 0; j < diff[i]; j++)
        {
            if (size == 1)
            {
                PscMemAbsSetByte(pbuf + len, g_psc_wave_data[i].buf[pre_idx[i]]);
            }
            else if (size == 2)
            {
                PscMemAbsSetWord(pbuf + len, g_psc_wave_data[i].buf[pre_idx[i]]);
            }
            else if (size == 4)
            {
                PscMemAbsSetDword(pbuf + len, g_psc_wave_data[i].buf[pre_idx[i]]);
            }

            pre_idx[i]  = (pre_idx[i] + 1) % PSC_WAVE_DATA_NUM_MAX;
            len        += size;
        }

        // 执行完这个时候，pre_idx[i]应该等于cur_idx[i]
        if (pre_idx[i] != cur_idx[i])
        {
            return;
        }

        g_psc_wave_list.channel[i].pre_idx = cur_idx[i];
    }

    // len
    PscMemAbsSetDword(pbuf + WAVE_MESS_HEAD_OFS, len);

    PscWaveSend(pbuf, len, PSC_WAVE_CLIENT_PROT);
}
volatile uint32_t buf_status = 0;
volatile uint32_t buf_status1 = 0;
/**
 * @brief             : 根据地址获取录波数据值
 * @attention         : 当中断执行完算法块后，调用此函数获取录波数据
 */
void PscGetWaveData(void)
{
    static uint32_t cnt_alone = 0; // 每个核自己的计数
    static uint32_t cache_cnt = 0;
    uint32_t       *buf = NULL;
    uint32_t        cur_idx;
    uint32_t        addr;
    uint32_t        size;
    uint32_t        port_id   = PORT_ID_BASE + PscGetCoreId();

    PscCacheInv(&g_psc_wave_list.state, 16); //刷16Byte的原因是想包含com字段
    if ((!g_psc_wave_list.state) || (g_psc_wave_list.com != port_id))
    {
        // 当录波功能未使能或者录波数据不是本核的数据
        return;
    }

    PscCacheInv(&g_psc_wave_list, sizeof(g_psc_wave_list));

    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        if (g_psc_wave_list.cnt[CORE_NR] > cnt_alone)
        {
            // 说明录波列表被清除过
            g_psc_wave_data[i].cur_idx = 0;
        }

        if (g_psc_wave_list.channel[i].use_flag == PSCTRUE)
        {
            buf     = g_psc_wave_data[i].buf;
            cur_idx = g_psc_wave_data[i].cur_idx;
            addr    = g_psc_wave_list.channel[i].addr;
            size    = g_psc_wave_list.channel[i].size;

            if(1 == size)
            {
                *((PSCBYTE*)&buf[cur_idx])  = *((PSCBYTE*)addr);
            }
            else if(2 == size)
            {
                *((PSCWORD*)&buf[cur_idx])  = *((PSCWORD*)addr);
            }
            else if(4 == size)
            {
                *((PSCDWORD*)&buf[cur_idx]) = *((PSCDWORD*)addr);
                // if(buf[cur_idx] == 0 && buf[(cur_idx - 1 + 200) % 200] == 0)
                // {
                //     buf_status++;
                // }
            }

            // cache_cnt++;
            // if (cache_cnt == 16)
            // {
            //     uint32_t index = (cur_idx - 16 + PSC_WAVE_DATA_NUM_MAX) % PSC_WAVE_DATA_NUM_MAX;
            //     PscCacheWb(&buf[index], 64);
            // }
           PscCacheWb(&buf[cur_idx], sizeof(buf[cur_idx]));
            g_psc_wave_data[i].cur_idx = (g_psc_wave_data[i].cur_idx + 1) % PSC_WAVE_DATA_NUM_MAX;
            // if (cache_cnt == 16)
            // {
            //     PscCacheWb(&g_psc_wave_data[i].cur_idx, sizeof(g_psc_wave_data[i].cur_idx));
            //     cache_cnt = 0;
            // }
           PscCacheWb(&g_psc_wave_data[i].cur_idx, sizeof(g_psc_wave_data[i].cur_idx));
//            PscCacheInv(g_psc_wave_data, sizeof(g_psc_wave_data));
            // if(buf[(cur_idx - 1 + 200) % 200] == 0)
            // {
            //     buf_status1++;
            // }
        }
    }

    cnt_alone = g_psc_wave_list.cnt[CORE_NR];
}

/**
 * @brief             : C6x获取录波数据
 * @attention         : 获取录波数据，并将录波数据从核内拷贝到片内临时缓存
 */
void PscGetWaveData_c6x(void)
{
    static uint32_t cnt_alone = 0; // 每个核自己的计数
    uint32_t       *buf       = NULL;
    uint32_t        addr;
    uint32_t        size;
    uint32_t        port_id   = PORT_ID_BASE + PscGetCoreId();
    static uint32_t num       = 0;
    static uint32_t tmp_index = 0;
    static psc_wave_data_tmp_info temp_info = {0};

    PscGetWaveStart();

    PscCacheInv(&g_psc_wave_list_c6x.state, 16); //刷16Byte的原因是想包含com字段
    if ((!g_psc_wave_list_c6x.state) || (g_psc_wave_list_c6x.com != port_id))
    {
        // 当录波功能未使能或者录波数据不是本核的数据
        return;
    }

    PscCacheInv(&g_psc_wave_list_c6x, sizeof(g_psc_wave_list_c6x));

    if (g_psc_wave_list_c6x.cnt[CORE_NR] > cnt_alone)
    {
        // 说明录波列表被清除过
        temp_info.clear_cnt       = g_psc_wave_list_c6x.cnt[CORE_NR];
        temp_info.cp_cnt          = 0;
        num                       = 0;
        tmp_index                 = 0;
    }

    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        if (g_psc_wave_list_c6x.channel[i].use_flag == PSCTRUE)
        {
            buf     = g_psc_wave_data_c6x[i].buf;
            addr    = g_psc_wave_list_c6x.channel[i].addr;
            size    = g_psc_wave_list_c6x.channel[i].size;

            if(1 == size)
            {
                *((PSCBYTE*)&buf[num])  = *((PSCBYTE*)addr);
            }
            else if(2 == size)
            {
                *((PSCWORD*)&buf[num])  = *((PSCWORD*)addr);
            }
            else if(4 == size)
            {
                *((PSCDWORD*)&buf[num]) = *((PSCDWORD*)addr);
            }
        }
    }

    num++;
    if (num >= PSC_WAVE_DATA_NUM_MAX_TMP)
    {
        num             = 0;
        temp_info.cp_cnt++;
        temp_info.index = (tmp_index + 1) % PSC_WAVE_DATA_TMP_NUM; //下一次要写到temp的索引
        PscCacheWb(&g_psc_wave_data_c6x, sizeof(g_psc_wave_data_c6x));
        PscGetWaveEnd(tmp_index, &temp_info);
        tmp_index       = (tmp_index + 1) % PSC_WAVE_DATA_TMP_NUM;
    }

    cnt_alone = g_psc_wave_list_c6x.cnt[CORE_NR];
}

/**
 * @brief      : 将C6x通过dma拷贝到临时缓存的数据转移到真正的录波数据表中
 * @attention  : 此函数只能R0调用，因为会涉及到写g_psc_wave_data的过程
 */
void PscWaveTmpTransfer(void)
{
    static uint32_t clear_cnt_alone       = 0; // C6x自己的计数
    static uint32_t cp_cnt_alone          = 0;
    static uint32_t index[PSC_WAVE_MAX_CHANNEL_NUM] = {0}; //真实录波列表中的下标索引

    uint32_t               tmp_index             = 0; // 从临时缓存中哪个下标中取数
    uint32_t               cp_num                = 0; // 从c6x核内到临时缓存一共拷贝的次数
    psc_wave_data_tmp_info temp_info;

    if ((g_psc_wave_list.state != PSCTRUE) || ((g_psc_wave_list.com != 5004) && (g_psc_wave_list.com != 5005)))
    {
        return;
    }

    PscCacheInv((uint8_t *)&g_psc_wave_temp_info, sizeof(g_psc_wave_temp_info));
    PscCacheInv((uint8_t *)&g_psc_wave_data_tmp, sizeof(g_psc_wave_data_tmp));
    temp_info.clear_cnt = g_psc_wave_temp_info.clear_cnt;
    temp_info.cp_cnt    = g_psc_wave_temp_info.cp_cnt;
    temp_info.index     = g_psc_wave_temp_info.index;

    if (temp_info.clear_cnt > clear_cnt_alone)
    {
        // 说明录波列表被清除过
        memset(index, 0, sizeof(index));
        cp_cnt_alone    = 0;
        clear_cnt_alone = temp_info.clear_cnt;
    }

    if (temp_info.cp_cnt >= cp_cnt_alone)
    {
        cp_num = temp_info.cp_cnt - cp_cnt_alone;
    }
    else
    {
        cp_num = cp_cnt_alone - temp_info.cp_cnt;
    }

    tmp_index = (temp_info.index + PSC_WAVE_DATA_TMP_NUM - cp_num) % PSC_WAVE_DATA_TMP_NUM;
    if (cp_num == 0)
    {
        return;
    }

    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        if (g_psc_wave_list.channel[i].use_flag == PSCFALSE)
        {
            continue;
        }

        for (uint32_t j = 0; j < cp_num; j++)
        {
            // 这个逻辑必须PSC_WAVE_DATA_NUM_MAX是PSC_WAVE_DATA_NUM_MAX_TMP的倍数，否则要考虑最后一个点拷贝的问题
            memcpy((uint8_t *)&g_psc_wave_data[i].buf[index[i]],
                   (uint8_t *)&g_psc_wave_data_tmp[(tmp_index + j) % PSC_WAVE_DATA_TMP_NUM][i].buf[0],
                   PSC_WAVE_DATA_NUM_MAX_TMP * sizeof(g_psc_wave_data[i].buf[index[i]]));
            index[i] = (index[i] + PSC_WAVE_DATA_NUM_MAX_TMP) % PSC_WAVE_DATA_NUM_MAX;
        }
    }

    cp_cnt_alone = temp_info.cp_cnt;

    for (uint8_t i = 0; i < PSC_WAVE_MAX_CHANNEL_NUM; i++)
    {
        if (g_psc_wave_list.channel[i].use_flag == PSCFALSE)
        {
            continue;
        }

        g_psc_wave_data[i].cur_idx = index[i];
    }

    PscCacheWb(g_psc_wave_data, sizeof(g_psc_wave_data));
}
