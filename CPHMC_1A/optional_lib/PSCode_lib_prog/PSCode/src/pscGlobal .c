/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscGlobal .c
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscGlobal.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
PSCBOOL fEnableCommunication = PSCFALSE;

/* resource version info */
tPscResVersion resVersion_g;        //单核资源版本信息
tPscResVersionTable resVersionMultiCore[PSC_CORE_NUM] = {0}; //多核资源版本信息

/* task settings (performance-critical for the T0 scheduler) */
tTaskConfig taskConfig_g;
/* for configuration data */

PSCBYTE gDataBuff[1*1024*1024];//1MB内存用于存储从上位机接收的数据

PSCBYTE Error_g = 0;

////TODO hjy: 目的在于骗过上位机编译器
/* for vartab data */
PSCBYTE** ppVartabSegments_g   = PSCNULL;
PSCWORD   wNumVartabSegments_g = PSCNULL;

unsigned char mode  = 0;     /* current mode - see enum tFBModes from tskScheduler.h */
PSCFLOAT dps_t0     = 0.0f;  /* T0 time cycle value in milliseconds (e.g. for 0.1 ms, dps_t0 is 0.1) */
PSCDWORD dps_t0_us  = 0;     /* T0 time cycle value in microseconds (e.g. for 0.1 ms, dps_t0_us is 100) - to avoid floating point inaccuracies */
PSCFLOAT dps_ta     = 0.0f;  /* sampling time ('ta') for the currently executing task, in milliseconds */
PSCFLOAT dps_t1_ta  = 0.0f;  /* 'ta' value of task T1, in milliseconds (calculated from T0 and the task's multiplication factor) */
PSCFLOAT dps_t2_ta  = 0.0f;  /* 'ta' value of task T2, in milliseconds (calculated from T0 and the task's multiplication factor) */
PSCFLOAT dps_t3_ta  = 0.0f;  /* 'ta' value of task T3, in milliseconds (calculated from T0 and the task's multiplication factor) */
PSCFLOAT dps_t4_ta  = 0.0f;  /* 'ta' value of task T4, in milliseconds (calculated from T0 and the task's multiplication factor) */
PSCFLOAT dps_t5_ta  = 0.0f;  /* 'ta' value of task T5, in milliseconds (calculated from T0 and the task's multiplication factor) */
PSCFLOAT dps_i1_ta  = 0.0f;  /* 'ta' value of task I1, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_i2_ta  = 0.0f;  /* 'ta' value of task I2, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_i3_ta  = 0.0f;  /* 'ta' value of task I3, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_i4_ta  = 0.0f;  /* 'ta' value of task I4, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_i5_ta  = 0.0f;  /* 'ta' value of task I5, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_i6_ta  = 0.0f;  /* 'ta' value of task I6, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_i7_ta  = 0.0f;  /* 'ta' value of task I7, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_i8_ta  = 0.0f;  /* 'ta' value of task I8, in milliseconds ("equivalent sampling time" configured by the user) */
PSCFLOAT dps_ta_bak = 0.0F;  /* to backup 'dps_ta' before giving a new value to it */
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
