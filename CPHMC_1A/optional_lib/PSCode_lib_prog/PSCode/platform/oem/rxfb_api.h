//###########################################################################
//
// File Name:   rxfb_api.h
//
// Description: rxfb api.
//
// Copyright:   Copyright (c) 2009
//              by Rongxin Power Electronic Co.Ltd China
//
//###########################################################################
//
// dd mmm yyyy | Who  | Description of changes
// ============|======|===============================================
// 28 Sep 2022 | ZHT  | Add Descriptions.
//
//###########################################################################

#ifndef _RXFB_API_H_
#define _RXFB_API_H_

/*---------------------------------------------------------------------------*/
/*                      Definition of data type                              */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/*                        Definition of constants                            */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                        Definition of interface                            */
/*---------------------------------------------------------------------------*/

void     fb_SetSysTick(uint32_t tick);
uint32_t fb_GetSysTick(void);

void fb_UpdateSysTick(void);

uint32_t fb_GetSysTickCnt(void);
void     fb_SetSysTickCnt(uint32_t tick);

uint32_t fb_GetSysTickMsCnt(void);

uint32_t str_to_tickCnt(char *str_time);

uint32_t fb_GetCpuCnt(void);

uint32_t fb_SetCpuCnt(void);

uint32_t fb_GetSysTime(uint16_t *year,
                       uint16_t *mon,
                       uint16_t *day,
                       uint16_t *hour,
                       uint16_t *min,
                       uint16_t *sec,
                       uint16_t *ms);

uint32_t *fb_getIsrTime(void);

uint32_t GetUsecTime(uint32_t count);
#endif  /* ifndef _RXFB_API_H_ */