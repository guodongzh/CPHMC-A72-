#ifndef _EXTERNAL_H_
#define _EXTERNAL_H_

#include "pscTypes.h"
#include "pscGlobal.h"
#include "pscEnv.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define LZSTRUE  0xFF
#define LZSFALSE 0x00
#define LZSNULL  0

/* 1 byte */
#define LZSBOOL     uint8_t
#define LZSCHAR     char
#define LZSSHORT    int8_t
#define LZSBYTE     uint8_t
/* 2 bytes */
#define LZSSINT     int16_t
#define LZSINT      int16_t
#define LZSUINT     uint16_t
#define LZSWORD     uint16_t
/* 4 bytes */
#define LZSLONG     int32_t
#define LZSDWORD    uint32_t
#define LZSFLOAT    float
/* 8 bytes */
#define LZSLONGLONG int64_t
#define LZSQWORD    uint64_t
#define LZSDOUBLE   double

#define LZSNEAR
#define LZSFAR
#define LZSCONST    const
#define LZSSTATIC   static
#define LZSPUBLIC
#define LZSPUBLIC32
#define LZSHUGE
#define LZSLARGE
#define _LDIV_SUPPORTED_

#define     GETREAL(addr)           *((LZSFLOAT*)(addr))
#define     GETBIT(addr)            (*((LZSBYTE*)(addr)) & 0x01 ? LZSTRUE : LZSFALSE)
#define     GETDWORD(addr)          *((LZSDWORD*)(addr))
#define     GETUDINT(addr)          *((LZSDWORD*)(addr))
#define     GETDINT(addr)           *((LZSLONG*)(addr))
#define     GETTIME(addr)           *((LZSDWORD*)(addr))
#define     GETUINT(addr)           *((LZSUINT*)(addr))
#define     GETWORD(addr)           *((LZSWORD*)(addr))
#define     GETINT(addr)            *((LZSINT*)(addr))
#define     GETBYTE(addr)           *((LZSBYTE*)(addr))

#define     SETREAL(addr, val)      {*((LZSFLOAT*)(addr)) = val;}
#define     SETBIT(addr, val)       {*((LZSBYTE*)(addr)) = val;}
#define     SETDWORD(addr, val)     {*((LZSDWORD*)(addr)) = val;}
#define     SETUDINT(addr, val)     {*((LZSDWORD*)(addr)) = val;}
#define     SETDINT(addr, val)      {*((LZSLONG*)(addr)) = val;}
#define     SETTIME(addr, val)      {*((LZSDWORD*)(addr)) = val;}
#define     SETUINT(addr, val)      {*((LZSUINT*)(addr)) = val;}
#define     SETWORD(addr, val)      {*((LZSWORD*)(addr)) = val;}
#define     SETINT(addr, val)       {*((LZSINT*)(addr)) = val;}
#define     SETBYTE(addr, val)      {*((LZSBYTE*)(addr)) = val;}

#define     PTR(addr)               ((LZSBYTE*)(addr))
#define     PTR_STRING(addr)        ((LZSCHAR*)(addr))

/* Definition of error codes */
typedef enum
{
    kLzsSuccess       = 0x00, /* everything OK */
} tLzsErrorCode;

typedef enum
{
    kInitMode         = 0,
    kSystemMode       = 1,
    kNormalMode       = 2,
    kFastImport       = 3,
    kConsistentImport = 4,
    kConsistentExport = 5,
    kFastExport       = 6
} tFBModes;

typedef enum
{
    kIecOK               = 0x00,     /* everything is fine */

    kIecGeneralError     = 0xFF,     /* general error */
    kIecFBNotSupported   = 0xFE,     /* FB is not implemented */
    kIecHardwareError    = 0xFD,     /* Error accessing hardware */

    /* [SYSTEC: 25.06.2003 -rs]: user-specific errorcode newly inserted */
    kIecOemError01       = 0x7F,     /* OEM specific error */
    kIecOemError02       = 0x7E,     /* OEM specific error */
    kIecOemError03       = 0x7D,     /* OEM specific error */
    kIecOemError04       = 0x7C,     /* OEM specific error */
    kIecOemError05       = 0x7B,     /* OEM specific error */
    kIecOemError06       = 0x7A,     /* OEM specific error */
    kIecOemError07       = 0x79,     /* OEM specific error */
    kIecOemError08       = 0x78,     /* OEM specific error */
    kIecOemError09       = 0x77,     /* OEM specific error */
    kIecOemError10       = 0x76,     /* OEM specific error */
    kIecOemError11       = 0x75,     /* OEM specific error */
    kIecOemError12       = 0x74,     /* OEM specific error */
    kIecOemError13       = 0x73,     /* OEM specific error */
    kIecOemError14       = 0x72,     /* OEM specific error */
    kIecOemError15       = 0x71,     /* OEM specific error */
    kIecOemError16       = 0x70,     /* OEM specific error */
    kIecOemError17       = 0x6F,     /* OEM specific error */
    kIecOemError18       = 0x6E,     /* OEM specific error */
    kIecOemError19       = 0x6D,     /* OEM specific error */
    kIecOemError20       = 0x6C      /* OEM specific error */

} tIecErrorCode;

/* for vartab data */
extern LZSBYTE** ppVartabSegments_g;
extern LZSWORD   wNumVartabSegments_g;

extern unsigned char mode;  /* current mode - see enum tFBModes from tskScheduler.h */
extern LZSFLOAT dps_t0;     /* T0 time cycle value in milliseconds (e.g. for 0.1 ms, dps_t0 is 0.1) */
extern LZSDWORD dps_t0_us;  /* T0 time cycle value in microseconds (e.g. for 0.1 ms, dps_t0_us is 100) - to avoid floating point inaccuracies */
extern LZSFLOAT dps_ta;     /* sampling time ('ta') for the currently executing task, in milliseconds */
extern LZSFLOAT dps_t1_ta;  /* 'ta' value of task T1, in milliseconds (calculated from T0 and the task's multiplication factor) */
extern LZSFLOAT dps_t2_ta;  /* 'ta' value of task T2, in milliseconds (calculated from T0 and the task's multiplication factor) */
extern LZSFLOAT dps_t3_ta;  /* 'ta' value of task T3, in milliseconds (calculated from T0 and the task's multiplication factor) */
extern LZSFLOAT dps_t4_ta;  /* 'ta' value of task T4, in milliseconds (calculated from T0 and the task's multiplication factor) */
extern LZSFLOAT dps_t5_ta;  /* 'ta' value of task T5, in milliseconds (calculated from T0 and the task's multiplication factor) */
extern LZSFLOAT dps_i1_ta;  /* 'ta' value of task I1, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_i2_ta;  /* 'ta' value of task I2, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_i3_ta;  /* 'ta' value of task I3, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_i4_ta;  /* 'ta' value of task I4, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_i5_ta;  /* 'ta' value of task I5, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_i6_ta;  /* 'ta' value of task I6, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_i7_ta;  /* 'ta' value of task I7, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_i8_ta;  /* 'ta' value of task I8, in milliseconds ("equivalent sampling time" configured by the user) */
extern LZSFLOAT dps_ta_bak; /* to backup 'dps_ta' before giving a new value to it */

#define SET_NOR_MODE() mode = kNormalMode;
#define SET_SYS_MODE() mode = kSystemMode;
#define SET_INI_MODE() mode = kInitMode;

/*BACKUP*/
#define TA_BACKUP()  dps_ta_bak = dps_ta
/*RESTORE*/
#define TA_RESTORE() dps_ta     = dps_ta_bak
/*GET*/
#define T1_TA_GET()  dps_ta     = dps_t1_ta
#define T2_TA_GET()  dps_ta     = dps_t2_ta
#define T3_TA_GET()  dps_ta     = dps_t3_ta
#define T4_TA_GET()  dps_ta     = dps_t4_ta
#define T5_TA_GET()  dps_ta     = dps_t5_ta
#define I1_TA_GET()  dps_ta     = dps_i1_ta
#define I2_TA_GET()  dps_ta     = dps_i2_ta
#define I3_TA_GET()  dps_ta     = dps_i3_ta
#define I4_TA_GET()  dps_ta     = dps_i4_ta
#define I5_TA_GET()  dps_ta     = dps_i5_ta
#define I6_TA_GET()  dps_ta     = dps_i6_ta
#define I7_TA_GET()  dps_ta     = dps_i7_ta
#define I8_TA_GET()  dps_ta     = dps_i8_ta

#define USE_PLATFORM_NAME
#define USE_PROJECT_PASSWORD

#endif
