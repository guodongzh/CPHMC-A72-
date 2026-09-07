/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       tfr_def.h
*@author     LiuRui
*@date       2025.07.23
*@brief
*@par        History
*Date        Version   Author     Description
*2025.07.23  1.0       LiuRui
******************************************************************************/

#ifndef TFR_DEF_H_
#define TFR_DEF_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "irig_b_interface.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define MAX_TFR_SIZE_ONE_CORE (0x2000000) // 32 MB

#define MAX_TFR_FILE_NUM_ONE_CORE (2)
#define MAX_TFR_INFO_SIZE (0x80) // 128 B, tfr head configration
#define MAX_TFR_DATA_BUF_SIZE (MAX_TFR_SIZE_ONE_CORE - MAX_TFR_INFO_SIZE) // tfr buffer


/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

/********************************************
 *          TFR_INF
 * ******************************************
 *          SAMP_HDR
 * ******************************************
 *
 *
 *
 *
 *          SAMP data
 *
 *
 *
 * ******************************************
 *
 */



// sample header, 16 bytes
typedef struct _SAMP_HDR{
    uint8_t trig; // rising edge trig, level trig
    uint8_t bak0;
    uint16_t year; // 2012-2050

    uint8_t month; // 1-12
    uint8_t day;   // 1-28,29,30,31
    uint8_t hour;  // 0-23
    uint8_t min;   // 0-59

    uint8_t sec; // 0-59
    uint8_t bak1;
    uint8_t bak2;
    uint8_t bak3;

    uint32_t us; // 0-999999
} SAMP_HDR_t;

// tfr infomation
typedef struct _TFR_INF{
    // cfg info
    uint8_t used;      // used is 1, defaut is 0
    uint8_t tfr_id;    // tfr id, 1, 2
    uint8_t edge_trig; // 1: rising edge trig; 0: level trig
    uint8_t cpu_id;    // core id, 1,2,3,4

    uint8_t res[3];  // reserved
    uint8_t buf_num; // number of buffers

    uint16_t ana_chan_num; // number of analog channels
    uint16_t dig_chan_num; // number of digtal channels

    uint32_t pre_samp_num; // number of pre fault samples
    uint32_t pos_samp_num; // number of post fault samples
    uint32_t samp_period;  // us( 10, 20, 50, 100, 1000, ...)

    // caculation info, 8-byte alignment
    uint32_t samp_size; //  ana_chan_num*2 + (dig_chan_num/16)*2 + sizeof(SAMP_HDR_t),
    uint32_t samp_num; // buf_num*(pre_samp_num + pos_samp_num)

    uint32_t buf_size; // buf_size  = samp_num*sample_size
    uint32_t buf_ofs;  // buf的起始偏移

    uint32_t ana_size; // ana size
    uint32_t ana_ofs;  // ana的起始偏移

    uint32_t dig_size; // dig size
    uint32_t dig_ofs;  // dig的起始偏移

    // run info
    uint32_t write_pos; // 当前写缓存index
    uint32_t cur_samp;  // 当前缓冲区地址

} TFR_INF_t;

typedef struct TFR_DATE
{
    uint16_t wYear;  /* 2012-2050     */
    uint8_t  byMon;  /* 1-12          */
    uint8_t  byDay;  /* 1-28,29,30,31 */
    uint8_t  byHour; /* 0-23          */
    uint8_t  byMin;  /* 0-59          */
    uint8_t  wSec;   /* 0-59      */
    uint16_t wMs;    /* 0-999       */
    uint16_t wUs;    /* 0-999       */
} TFR_DATE_t;


/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/



/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
void tfr_init(uint8_t id, uint8_t nbuf, uint16_t nAnaCh, uint16_t nDigCh,
              uint32_t preFault, uint32_t postFault, uint32_t samp_period,
              uint8_t *errCode, uint32_t *samp_size, uint32_t *buf_size,
              uint32_t *tfr_size, uint32_t *ana_size, uint32_t *dig_size);

void tfr_update(uint8_t id);

void tfr_trig(uint8_t id, uint8_t trig);

void tfr_ana_4r(uint8_t id, uint8_t blkNr, float k1, float v1, float k2,
                float v2, float k3, float v3, float k4, float v4);

void tfr_dig_1w(uint8_t id, uint8_t blkNr, uint16_t v1);
void tfr_dig_16b(uint8_t id, uint8_t blkNr, uint8_t v1, uint8_t v2, uint8_t v3,
                 uint8_t v4, uint8_t v5, uint8_t v6, uint8_t v7, uint8_t v8,
                 uint8_t v9, uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13,
                 uint8_t v14, uint8_t v15, uint8_t v16);


#ifdef __cplusplus
}
#endif

#endif //_tfr_def



