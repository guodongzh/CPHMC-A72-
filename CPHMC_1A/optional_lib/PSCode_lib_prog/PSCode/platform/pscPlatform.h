/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscPlatform.h
 *@author     LiuRui
 *@date       2026.02.03
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.02.03  1.0       LiuRui
 ******************************************************************************/

#ifndef __PSCPLATFORM_H
#define __PSCPLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <ti/osal/CacheP.h>
#include "bsp_init.h"
#include "udma_mem_copy.h"
#include "platform.h"
#include "pscTypes.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
extern char g_hardware_name[];
extern char g_platform_name[];
extern tPscSetData *setDataTablePtr;
extern PSCDWORD *pWatchData;
extern tPscWatchTable *pWatchTable;


#if defined(SOC_J721E)
#define PORT_ID_BASE 5000
#define PSC_CORE_NUM 8

#define PLATFORM_NAME "CPHMC_1A"

#if defined(BUILD_MCU2_0)
#define CORE_NAME            "R5F0"
#define CORE_NR              0
#define PLATFORM_CPHMC_1A_R0 /* add your own plaform definitions, if required */

#elif defined(BUILD_MCU2_1)
#define CORE_NAME            "R5F1"
#define CORE_NR              1
#define PLATFORM_CPHMC_1A_R1 /* add your own plaform definitions, if required */

#elif defined(BUILD_MCU3_0)
#define CORE_NAME            "R5F2"
#define CORE_NR              2
#define PLATFORM_CPHMC_1A_R2 /* add your own plaform definitions, if required */

#elif defined(BUILD_MCU3_1)
#define CORE_NAME            "R5F3"
#define CORE_NR              3
#define PLATFORM_CPHMC_1A_R3 /* add your own plaform definitions, if required */

#elif defined(BUILD_C66X_1)
#define CORE_NAME             "C60"
#define CORE_NR               4
#define PLATFORM_CPHMC_1A_C60 /* add your own plaform definitions, if required */

#elif defined(BUILD_C66X_2)
#define CORE_NAME             "C61"
#define CORE_NR               5
#define PLATFORM_CPHMC_1A_C61 /* add your own plaform definitions, if required */

#elif defined(BUILD_C7X_1)
#define CORE_NAME             "C7x"
#define CORE_NR               6
#define PLATFORM_CPHMC_1A_C70 /* add your own plaform definitions, if required */

#elif defined(BUILD_MCU1_1)

#define CORE_NAME            "MCU1"
#define CORE_NR              7
#define PLATFORM_CPHMC_1A_U1 /* add your own plaform definitions, if required */

#endif

#elif defined(SOC_AM64X)
#define PORT_ID_BASE  5000
#define PSC_CORE_NUM  4

extern char g_core_name[PSC_CORE_NUM][VERSION_STRING_SIZE / 2];

#define PLATFORM_NAME "CPEMC_2A"

#ifdef CORE_R5F0
#define CORE_NAME "R5F0"
#define CORE_NR   0
#endif

#ifdef CORE_R5F1
#define CORE_NAME            "R5F1"
#define CORE_NR              1
#define PLATFORM_CPEMC_2A_R1 /* add your own plaform definitions, if required */

#endif

#ifdef CORE_R5F2
#define CORE_NAME            "R5F2"
#define CORE_NR              2
#define PLATFORM_CPEMC_2A_R2 /* add your own plaform definitions, if required */

#endif

#ifdef CORE_R5F3
#define CORE_NAME            "R5F3"
#define CORE_NR              3
#define PLATFORM_CPEMC_2A_R3 /* add your own plaform definitions, if required */

#endif

#endif

#define PSC_WAVE_DATA_NUM_MAX_TMP      5
#define PSC_WAVE_DATA_TMP_NUM          20
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
typedef struct
{
    uint32_t      buf[PSC_WAVE_DATA_NUM_MAX_TMP];     // 褰曟尝鏁版嵁缂撳瓨
}psc_wave_data_tmp;

typedef struct
{
    uint32_t      clear_cnt; //褰曟尝鍒楄〃琚竻闆剁殑娆℃暟
    uint32_t      cp_cnt;    //c6x鐨勫綍娉㈡暟鎹悜temp鎷疯礉鐨勬鏁
    uint32_t      index;
}psc_wave_data_tmp_info;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
uint8_t PscGetCoreId(void);
void PscCacheInv(const void *addr, uint32_t size);
void PscCacheWb(const void *addr, uint32_t size);
void PscSetVariableStart();
void PscSetVariableEnd();
void PscGetWatchDataStart();
void PscGetWatchDataEnd();
void PscGetWaveStart();
void PscGetWaveEnd(uint32_t tmp_index, psc_wave_data_tmp_info *info);
void PscResVersionShare(void);
void PscGetResVersionShare(void);

extern tPscResVersionTable resVersionMultiCore[PSC_CORE_NUM];
#ifdef __cplusplus
}
#endif

#endif  //__PSCPLATFORM_H
