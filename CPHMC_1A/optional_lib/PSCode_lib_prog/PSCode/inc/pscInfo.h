/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscInfo.h
 *@author     jinyangh
 *@date       2025.02.26
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.02.26  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCINFO_H
#define _PSCINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscBase.h"
#include "pscState.h"
#include "pscTypes.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
/* errorcodes of the interpreter ( returncode of IpCycle to PSC ) */
typedef enum
{
    kIpOK                    = 0x00, /* no error */
    kIpTaskCmdInvalid        = 0x9F, /* unknown control command */
    kIpOpcodeInvalid         = 0x9E, /* unused (free) opcode */
    kIpOpcodeNotSupported    = 0x9D, /* opcode not implemented */
    kIpExtensionInvalid      = 0x9C, /* unused (free) extension */
    kIpDivisionByZero        = 0x9B, /* division by zero */
    kIpArrayIndexInvalid     = 0x9A, /* invalid index for array */
    kIpFirmwareExecError     = 0x99, /* error executing firmware function block */
    kIpPflowNotAvailable     = 0x98, /* Powerflow not available */
    kIpInvalidBitRef         = 0x97, /* invalid bit reference*/
    kIpErrorRestoreData      = 0x96, /* error in restore data*/
    kIpNoValidArrElementSize = 0x95, /* select array index wrong element size*/
    kIpInvalidStructSize     = 0x94, /* length in struct header invalid */
    kIpModuloZero            = 0x93, /* second operand to mod was zero, result undefined */
    kIpArrElemNotSupported   = 0x92, /* array elements of unknown type */
    kIpNoMem                 = 0x91, /* out of memory */
    kIpInvalidTypecast       = 0x90, /* unsupported typecast */
    kIpCPUException          = 0x8F  /* CPU exception */

    /* until 0x80: reserved for future runtime error codes */

} tIpErrorCode;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
PSCBYTE PscInfGetError();
tPscErrTabEntry const* PscInfGetErrorEntry(PSCBYTE bPlcErrCode_p);
tPscErrTabEntry const *PscEnvGetOemPscError(PSCBYTE bPscErrorCode_p);
PSCBYTE PscIpGetIpErrCode(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCINFO_H */
