/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscWave.h
 *@author     jinyangh
 *@date       2026.02.25
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.02.25  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCWAVE_H
#define _PSCWAVE_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscCom.h"
#include "pscPlatform.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define PSC_WAVE_MAX_CHANNEL_NUM    8            // 暂定8个录波通道
#define PSC_WAVE_DATA_NUM_MAX       200          // 每个通道缓存数量(每个通道最大缓存点数)
#define PSC_WAVE_TYPE               0x10         // 录波标志
#define PSC_WAVE_CLIENT_PROT        4999         // 录波数据发送到上位机的端口号

#define WAVE_MESS_HEAD_OFS          4   // 报文头占位大小
#define WAVE_MESS_LEN_OFS           4   // 报文长度占位大小
#define WAVE_MESS_VERSION_OFS       2   // 协议版本占位大小
#define WAVE_MESS_CNT_OFS           8   // 计数器占位大小
#define WAVE_MESS_TIME_OFS          4   // 时间片占位大小
#define WAVE_MESS_DATA_ADD_OFS      8   // 数据地址占位大小
#define WAVE_MESS_ISR_OFS           2   // 采集间隔时间占位大小
#define WAVE_MESS_DATA_NUM_OFS      2   // 点数占位大小
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct
{
    uint8_t       use_flag;     // 此通道是否已经被使用
    uint16_t      pgm;          // 录波数据的程序编号
    uint32_t      addr;         // 录波数据的地址
    uint16_t      size;         // 录波数据的大小
    uint32_t      pre_idx;      // 上一次数据在缓存中的索引，因为这个是R0写，所以放在此处
}psc_wave_channel;

typedef struct
{
    uint8_t             state;                               // 当前状态是否能正常录波
    uint32_t            com;                                 // 录波列表中的数据对应的端口号（用于区分不同核的录波数据信息）
    uint32_t            used_channel_num;                    // 已使用的通道数
    uint32_t            cnt[PSC_CORE_NUM];                   // 当录波列表发生改变后，cnt加1，用于同步修改录波缓存的最新数据下标
    psc_wave_channel    channel[PSC_WAVE_MAX_CHANNEL_NUM];   // 录波通道
}psc_wave;

typedef struct
{
    uint32_t      buf[PSC_WAVE_DATA_NUM_MAX];     // 录波数据缓存
    uint32_t      cur_idx;                        // 最新数据在缓存中的索引
}psc_wave_data;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern psc_wave         g_psc_wave_list; // R0写，其他核读
extern psc_wave_data    g_psc_wave_data[PSC_WAVE_MAX_CHANNEL_NUM]; // 其他核写，R0读
extern psc_wave               g_psc_wave_list_c6x; // MSMC -> C6x
extern psc_wave_data_tmp      g_psc_wave_data_c6x[PSC_WAVE_MAX_CHANNEL_NUM]; // C6x -> MSMC
extern psc_wave_data_tmp      g_psc_wave_data_tmp[PSC_WAVE_DATA_TMP_NUM][PSC_WAVE_MAX_CHANNEL_NUM];
extern psc_wave_data_tmp_info g_psc_wave_temp_info;

void PscWaveInit(void);
void PscWaveDiscard(void);
void PscAddWaveList(tPscDataAdr * pWatchList);
void PscWaveEnable(void);
void PscDrawWave(void);
void PscGetWaveData(void);
void PscGetWaveData_c6x(void);
void PscWaveTmpTransfer(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCWAVE_H */