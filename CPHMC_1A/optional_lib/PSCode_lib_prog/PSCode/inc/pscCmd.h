/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscCmd.h
 *@author     jinyangh
 *@date       2025.01.07
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.07  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCCMD_H
#define _PSCCMD_H

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
/******************************* Query firmware library information *******************/
#define PLC_FIRMWARE_SIZE (1)

/******************************* Query firmware version *******************************/
#define PLC_VERSION_SIZE         (712)
#define VERSION_STRING_SIZE      (32)

#define HARDWARE_DESCRIPTION g_hardware_name
#define PLATFORM_DESCRIPTION g_platform_name

//offset
#define STRUCT_PLCVER_NAME            0         /* 60    chars */
#define STRUCT_PLCVER_HW              60        /* 2     chars */
#define STRUCT_PLCVER_LOWLIB          62        /* 4     chars */
#define STRUCT_PLCVER_APP             66        /* 320   chars */
#define STRUCT_PLCVER_FW              386       /* 320   chars */
#define STRUCT_PLCVER_FPGASOFT        706       /* 2     chars */
#define STRUCT_PLCVER_FPGAFUN         708       /* 2     chars */

#define COMBINE_TO_STRING_POINT_2B(str_param, num_param, buffer)     \
    do {                                                             \
        uint16_t __v = (uint16_t)(num_param);                        \
        uint8_t __v0 = (__v >> 8) & 0xFF;                            \
        uint8_t __v1 =  __v       & 0xFF;                            \
                                                                     \
        snprintf((buffer), VERSION_STRING_SIZE, "%s:%u.%u",          \
                 (str_param), __v0, __v1);                           \
    } while (0)

#define COMBINE_TO_STRING_POINT_4B(str_param, num_param, buffer)     \
    do {                                                             \
        uint32_t __v = (uint32_t)(num_param);                        \
        uint8_t __v0 = (__v >> 24) & 0xFF;                           \
        uint8_t __v1 = (__v >> 16) & 0xFF;                           \
        uint8_t __v2 = (__v >>  8) & 0xFF;                           \
        uint8_t __v3 =  __v        & 0xFF;                           \
                                                                     \
        snprintf((buffer), VERSION_STRING_SIZE, "%s:%u.%u.%u.%u",    \
                 (str_param), __v0, __v1, __v2, __v3);               \
    } while (0)


/******************************* Query project version ********************************/
#define RES_VERSION_SIZE (1 + 216)

//offset
#define STRUCT_OFFSET_RES_PRJNAME               0   /* 32 chars */
#define STRUCT_OFFSET_RES_RESNAME              32   /* 32 chars */
#define STRUCT_OFFSET_RES_PLCVER               64   /* DWORD */
#define STRUCT_OFFSET_RES_BUILDDATE            68   /* DWORD */
#define STRUCT_OFFSET_RES_LOADDATE             72   /* DWORD */
#define STRUCT_OFFSET_RES_PLATFORMNAME         76   /* 32 chars */
#define STRUCT_OFFSET_RES_PASSWORD_DEVELOPER  108   /* 36 bytes */
#define STRUCT_OFFSET_RES_PASSWORD_PROJECT    144   /* 36 bytes */
#define STRUCT_OFFSET_RES_PASSWORD_CUSTOMER   180   /* 36 bytes */
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern PSCBYTE      g_core_name[PSC_CORE_NUM][VERSION_STRING_SIZE / 2];

PSCBYTE PscCtlLogin();
PSCBYTE PscCtlLogout();
PSCBYTE PscEnvRebootPlc();
PSCBYTE PscInfGetFwLibInfo();
PSCBYTE PscInfGetPlcVersion();
PSCBYTE PscInfGetResVersion();
PSCBYTE PscCsvSendRequestedData();
PSCBYTE PscCtlSetState(tPscPSCmd *pPscPSCmd_p);
void    PscInfoChange(PSCDWORD prot_id);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCCMD_H */
