// ###########################################################################
//
//  File Name:   rxfb_api.c
//
//  Description: rxfb api.
//
//  Copyright:   Copyright (c) 2009
//               by Rongxin Power Electronic Co.Ltd China
//
// ###########################################################################
//
//  dd mmm yyyy | Who  | Description of changes
//  ============|======|===============================================
//  28 Sep 2022 | ZHT  | Add Descriptions.
//
// ###########################################################################
#include <stdint.h>
#include "stdio.h"
#include "string.h"
#include "rxfb_api.h"
#include "irig_b_interface.h"

volatile uint32_t ISR_TimeValues[4] = {0};
uint32_t            g_fbSysTickCnt    = 0;  // system tick, schedual interrupt update
uint32_t            g_fbSysTickUs     = 100;

void fb_SetSysTick(uint32_t tick) { g_fbSysTickUs = tick; }

uint32_t fb_GetSysTick(void) { return g_fbSysTickUs; }

void fb_UpdateSysTick(void) { g_fbSysTickCnt++; }

uint32_t fb_GetSysTickCnt(void) { return g_fbSysTickCnt; }

void fb_SetSysTickCnt(uint32_t tick) { g_fbSysTickCnt = tick; }

uint32_t fb_GetSysTickMsCnt(void) { return (g_fbSysTickCnt * g_fbSysTickUs / 1000); }

// 字符串转换为中断计数
// str_time: T#1us, T#1ms, T#1s, T#1min, T#1hour, T#1day
uint32_t str_to_tickCnt(char *str_time)
{
    char   sUnit[10];
    int    ntm;
    uint32_t tickCnt;

    sscanf(str_time, "T#%d%s", &ntm, sUnit);
    if (ntm < 0)
    {
        return 0;
    }

    if (strcmp(sUnit, "us") == 0)  // us
    {
        tickCnt = ntm / g_fbSysTickUs;
    }
    else if (strcmp(sUnit, "ms") == 0)  // ms
    {
        tickCnt = ntm * 1000 / g_fbSysTickUs;
    }
    else if (strcmp(sUnit, "s") == 0)  // s
    {
        tickCnt = ntm * 1000 * 1000 / g_fbSysTickUs;
    }
    else if (strcmp(sUnit, "min") == 0)  // min
    {
        tickCnt = ntm * 60 * 1000 * 1000 / g_fbSysTickUs;
    }
    else if (strcmp(sUnit, "hour") == 0)  // hour
    {
        tickCnt = ntm * 60 * 60 * 1000 * 1000 / g_fbSysTickUs;
    }
    else if (strcmp(sUnit, "day") == 0)  // day
    {
        tickCnt = ntm * 24 * 60 * 60 * 1000 * 1000 / g_fbSysTickUs;
    }
    else
    {
        tickCnt = 0;
    }

    return tickCnt;
}

uint32_t fb_GetCpuCnt(void)
{
    return 0;
}

uint32_t fb_SetCpuCnt(void) { return 0u; }

uint32_t fb_GetSysTime(uint16_t *year,
                     uint16_t *mon,
                     uint16_t *day,
                     uint16_t *hour,
                     uint16_t *min,
                     uint16_t *sec,
                     uint16_t *ms)
{
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

    *year = StruTime_dest.Year;
    *mon = StruTime_dest.Month;
    *day = StruTime_dest.Date;
    *hour = StruTime_dest.Hour;
    *min = StruTime_dest.Min;
    *sec = StruTime_dest.Sec;
    *ms  = StruTime_dest.uSec_100 * 0.1;

//    *year = 2022;
//    *mon  = 11;
//    *day  = 22;
//    *hour = 16;
//    *min  = 37;
//    *sec  = 34;
//    *ms   = 300;
    return (0);
}

uint32_t *fb_getIsrTime(void)
{
    return (uint32_t *)&ISR_TimeValues[0];
}