/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscDownload.h
 *@author     jinyangh
 *@date       2025.02.18
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.02.18  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCDOWNLOAD_H
#define _PSCDOWNLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscBase.h"
#include "pscTypes.h"
#include "pscCom.h"
#include "norflash_fw_writer.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern PSCBYTE g_bRawFileType;

/* for CRC checksums */
extern PSCBOOL bCRCLookupInitialized;
extern PSCDWORD crcLookupTable[0x100];
extern PSCDWORD dwCRCChecksumFromPS;
extern PSCDWORD dwCRCChecksumFromDwl;

void PscInitializeCRCLookup();
void PscEnvAddCRCByte(PSCDWORD *pdwCurrentCRCValue, PSCBYTE b);
PSCBYTE PscEnvDwlRawFile(tPscPSCmd *pPscPSCmd_p);
PSCBYTE PscEnvDwlRawFileSegment(tPscPSCmd *pPscPSCmd_p, PSCBOOL fLastSegment);
PSCBYTE PscEnvDwlContRawFileSegment(tPscPSCmd *pPscPSCmd_p);
PSCBYTE PscEnvSetFileTimeOfRawFile(tPscPSCmd *pPscPSCmd_p);
PSCBYTE WriteFirmwareDataIntoFlash(PSCBYTE *pBuffer, PSCWORD wSegSize, PSCBOOL fLastSegment);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCDOWNLOAD_H */
