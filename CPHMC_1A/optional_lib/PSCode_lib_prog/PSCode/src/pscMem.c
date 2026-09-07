/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscMem.c
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
#include "pscMem.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/*------------< write byte to absolute address >-------------------------- */
void PscMemAbsSetByte(PSCBYTE* pAddr_p, PSCBYTE ByteVal_p)
{

    /* This function is used to write a byte to an absolute given address.
       The functions of the group <PscMemAbsSetxxx> are used by the RTS to store
       data in intel-format used by the IDE. (e.g. segment header)
                                                                               */

    *pAddr_p = ByteVal_p;
}

/*------------< write word to absolute address >-------------------------- */
void PscMemAbsSetWord(PSCBYTE* pAddr_p, PSCWORD WordVal_p)
{
    /* This function is used to write a word to an absolute given address.
       The functions of the group <PscMemAbsSetxxx> are used by the RTS to store
       data in intel-format used by the IDE. (e.g. segment header)
                                                                               */
    *pAddr_p     = PSCLOBYTE(WordVal_p);
    *(pAddr_p+1) = PSCHIBYTE(WordVal_p);
}

/*------------< write dword to absolute address >------------------------- */
void PscMemAbsSetDword(PSCBYTE* pAddr_p, PSCDWORD DwordVal_p)
{
    /* This function is used to write a dword to an absolute given address.
       The functions of the group <PscMemAbsSetxxx> are used by the RTS to store
       data in intel-format used by the IDE. (e.g. segment header)
                                                                               */
    *pAddr_p   	 = PSCLOBYTE(PSCLOWORD(DwordVal_p));
    *(pAddr_p+1) = PSCHIBYTE(PSCLOWORD(DwordVal_p));
    *(pAddr_p+2) = PSCLOBYTE(PSCHIWORD(DwordVal_p));
    *(pAddr_p+3) = PSCHIBYTE(PSCHIWORD(DwordVal_p));
}

/*------------< read byte from absolute address >--------------------------- */
PSCBYTE PscMemAbsGetByte(PSCBYTE* pAddr_p)
{
    /* This function is used to read a byte from an absolute given address.
       The functions of the group <PscMemAbsGetxxx> are used by the RTS to read
       data transferred from the IDE, stored in intel-format. (e.g. segment header)
                                                                               */

    return ( *(PSCBYTE *)pAddr_p );
}

/*------------< read word from absolute address >--------------------------- */
PSCWORD PscMemAbsGetWord(PSCBYTE* pAddr_p)
{
    /* This function is used to read a word from an absolute given address.
       The functions of the group <PscMemAbsGetxxx> are used by the RTS to read
       data transferred from the IDE, stored in intel-format. (e.g. segment header)
                                                                                  */
    PSCWORD rc=0;

    rc	= 		(PSCWORD)(*(PSCBYTE *) ((PSCDWORD)(pAddr_p+1  )));
    rc 	= (rc <<8) +(PSCWORD)(*(PSCBYTE *) ((PSCDWORD)(pAddr_p    )));

    return rc;
}

/*------------< read dword from absolute address >-------------------------- */
PSCDWORD PscMemAbsGetDword(PSCBYTE* pAddr_p)
{
    /* This function is used to read a dword from an absolute given address.
       The functions of the group <PscMemAbsGetxxx> are used by the RTS to read
       data transferred from the IDE, stored in intel-format. (e.g. segment header)
                                                                               */

    PSCDWORD rc=0;
    rc = 			(PSCDWORD)(*(PSCBYTE *) ((PSCDWORD)(pAddr_p+3)));
    rc =(rc <<8) + 	(PSCDWORD)(*(PSCBYTE *) ((PSCDWORD)(pAddr_p+2)));
    rc =(rc <<8) + 	(PSCDWORD)(*(PSCBYTE *) ((PSCDWORD)(pAddr_p+1)));
    rc =(rc <<8) + 	(PSCDWORD)(*(PSCBYTE *) ((PSCDWORD)(pAddr_p  )));

    return rc;
}