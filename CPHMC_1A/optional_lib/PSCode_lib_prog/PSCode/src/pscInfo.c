/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscInfo.c
 *@author     jinyangh
 *@date       2025.02.26
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.02.26  1.0       jinyangh    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscInfo.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/*  String constants for error messages */
const PSCCHAR s0[]    = "Controller working normal";
const PSCCHAR s1[]    = "GENERAL ERROR";
const PSCCHAR s2[]    = "UNSPECIFIED ERROR";

const PSCCHAR s1001[] = "Local Run/Stop-Switch on PLC set to <STOP>";
const PSCCHAR s1002[] = "Out of program memory. Program execution not possible.";
/* const PSCCHAR s1003[] = "Hardware error"; */
const PSCCHAR s1004[] = "No valid program";
const PSCCHAR s1005[] = "Download of invalid data";
const PSCCHAR s1006[] = "Configuration error/wrong program";
/* const PSCCHAR s1007[] = "Module configuration error"; */
const PSCCHAR s1008[] = "Invalid program number";
const PSCCHAR s1009[] = "Invalid segment number";
const PSCCHAR s1010[] = "Invalid segment type";
const PSCCHAR s1011[] = "Segment already on PLC";
const PSCCHAR s1012[] = "No free watch ID available";
const PSCCHAR s1013[] = "Invalid command received";
const PSCCHAR s1014[] = "Action not valid. Wrong mode.";
const PSCCHAR s1015[] = "General network error";
const PSCCHAR s1016[] = "Accepted receipt too small";
/* const PSCCHAR s1017[] = "Error reading/writing process image"; */
/* const PSCCHAR s1018[] = "Timertask error"; */
/* const PSCCHAR s1019[] = "Wrong kernel version"; */
const PSCCHAR s1020[] = "Error calling kernal";
const PSCCHAR s1021[] = "Error calling native code";
/* const PSCCHAR s1022[] = "Out of backup memory (EEPROM/Flash). Program will be lost on power down."; */
/* const PSCCHAR s1023[] = "Error in I/O-Configuration"; */
/* const PSCCHAR s1024[] = "Out of user disk space. Download of raw file failed."; */
const PSCCHAR s1025[] = "Invalid action. Switch PLC to stop first.";
/* const PSCCHAR s1101[] = "RUNTIME ERROR: cycle length exceeded"; */
/* const PSCCHAR s1102[] = "RUNTIME ERROR: RTX BaseTimer length exceeded"; */
const PSCCHAR s1103[] = "The previous online session was interrupted unexpectedly";
const PSCCHAR s1104[] = "UPLOAD ERROR: resource does not contain upload information";
const PSCCHAR s1105[] = "No free hist ID avaialable";
const PSCCHAR s1106[] = "Invalid hist ID";
const PSCCHAR s1107[] = "Program memory corrupted";
const PSCCHAR s1201[] = "Writing of RawFile failed (disk full, write protected etc.)";
const PSCCHAR s1202[] = "Reading of RawFile failed (file does not exist, no permission, etc.). Try to download again.";
const PSCCHAR s1203[] = "Deleting of RawFile failed (file is r/o, no permission etc.)";
/* const PSCCHAR s1501[] = "Network configuration error"; */
/* const PSCCHAR s1502[] = "Network error in IO-Process communication"; */
/* const PSCCHAR s1503[] = "Invalid Node Address selected for this PLC"; */
/* const PSCCHAR s1504[] = "Invalid configuration for Network Variables (incorrect DCF)"; */
/* const PSCCHAR s1505[] = "Network Image overflow (too many network variables defined)"; */
/* const PSCCHAR s1506[] = "Invalid IP configuration selected (MAC-Addr, IP-Addr, Subnet-Mask)"; */
/* const PSCCHAR s1507[] = "Remote Node configuration error for details see Error Logfile"; */
/* const PSCCHAR s1508[] = "A network error was occured during the PLC offline state"; */
const PSCCHAR s1601[] = "No breakpoint";
const PSCCHAR s1602[] = "Maximum number of breakpoints reached";
const PSCCHAR s1603[] = "Breakpoint not found";
const PSCCHAR s1604[] = "TDT error";
const PSCCHAR s1605[] = "Error moving segment";
const PSCCHAR s1606[] = "Linker table error";
const PSCCHAR s1607[] = "Alignment error";
const PSCCHAR s1608[] = "DS size error";
const PSCCHAR s1609[] = "Error reading segment address";
const PSCCHAR s1610[] = "Resource replace error";
const PSCCHAR s1611[] = "No segment table";
const PSCCHAR s1612[] = "Download procdata error";
const PSCCHAR s1613[] = "No copy table";
const PSCCHAR s1614[] = "Maximum number of history entries reached";
const PSCCHAR s1615[] = "Historical data size error";
const PSCCHAR s1616[] = "Historical data mutex error";
const PSCCHAR s1617[] = "PSC_MAXHIST is set too high";
const PSCCHAR s1618[] = "Unsupported force type";
const PSCCHAR s1619[] = "Unsupported watch type";
const PSCCHAR s1620[] = "Error deleting watch entry";
/* const PSCCHAR s1621[] = "Interpreter error"; */
/* const PSCCHAR s1622[] = "Parameter value error"; */
const PSCCHAR s1623[] = "Process image error";
const PSCCHAR s1624[] = "Status error on login";
const PSCCHAR s1625[] = "Status error on logout";
const PSCCHAR s1626[] = "Error writing segment address";
const PSCCHAR s1627[] = "Error saving temporary segment";
/* const PSCCHAR s1628[] = "NC firmware execution error"; */
/* const PSCCHAR s1629[] = "Undefined stub called in native code"; */
const PSCCHAR s1630[] = "Instance stack overflow";
const PSCCHAR s1631[] = "Instance stack underflow";
const PSCCHAR s1632[] = "CRC Error reading persistence";
const PSCCHAR s1633[] = "Version mismatch between target system and persistence";
const PSCCHAR s1634[] = "Saving persistence failed!";
const PSCCHAR s1635[] = "Error getting write buffer!";
const PSCCHAR s1636[] = "Fatal cycle error. See error log for details.\n\nThe respective system task was deleted in order to keep online communication possible.\nCPU load of the application must be reduced.\nThe CPU can be started again only after a new download.";
const PSCCHAR s1637[] = "No hardware configuration segment found";
const PSCCHAR s1638[] = "Error during code generation for maximum speed optimization:\nUnsupported instruction.\nPlease configure \"Speed\" or \"Size\" optimization to run this application!";
const PSCCHAR s1639[] = "Error during code generation for maximum speed optimization:\nInvalid function block call.";
const PSCCHAR s1640[] = "Error during code generation for maximum speed optimization:\nUnsupported combination of instructions.\nPlease configure \"Speed\" or \"Size\" optimization to run this application!";
const PSCCHAR s1641[] = "RUNTIME ERROR: error during execution of BL code";
const PSCCHAR s1642[] = "No shared memory configuration segment found";
const PSCCHAR s1643[] = "Error in shared memory configuration";
const PSCCHAR s1644[] = "Error initializing shared memory";
const PSCCHAR s1645[] = "Error connecting to shared memory";
const PSCCHAR s1646[] = "Error setting up CPU-local shared memory information";
const PSCCHAR s1647[] = "Error getting write buffer in shared memory!";
const PSCCHAR s1648[] = "Error: size of consistent shared memory is too small!\nEither remove some shared memory variables,\nor enlarge the consistent shared memory.";
const PSCCHAR s1649[] = "Error: shared memory configuration is different from the master CPU!\nThis CPU has been stopped.\n\nTo run this CPU again, its application must be rebuilt and redownloaded.";
const PSCCHAR s1650[] = "Error: recursion in user FBs (not allowed).";
const PSCCHAR s1651[] = "Error during hardware detection. See error log for details.";
const PSCCHAR s1652[] = "Error during apply hardware configuration settings. See error log for details.";
const PSCCHAR s1653[] = "Error on the master CPU during hardware check and configuration. See error log of the master CPU for details.";
const PSCCHAR s1654[] = "Error: hardware configuration is different from the master CPU!\nThis CPU has been stopped.\n\nTo run this CPU again, its application must be rebuilt and redownloaded.";
const PSCCHAR s1655[] = "T0 watchdog has timed out. Cyclic tasks might not be running any more.";
const PSCCHAR s1656[] = "No more free entry in the dynamic retain table. The value could not be made persistent.";
const PSCCHAR s2001[] = "RUNTIME ERROR: division by zero";
const PSCCHAR s2002[] = "RUNTIME ERROR: invalid array index";
const PSCCHAR s2003[] = "RUNTIME ERROR: invalid opcode";
const PSCCHAR s2004[] = "RUNTIME ERROR: opcode not supported";
const PSCCHAR s2005[] = "RUNTIME ERROR: invalid extension";
const PSCCHAR s2006[] = "RUNTIME ERROR: unknown command";
const PSCCHAR s2007[] = "RUNTIME ERROR: kernel without powerflow";
const PSCCHAR s2008[] = "RUNTIME ERROR: invalid bit reference";
const PSCCHAR s2009[] = "RUNTIME ERROR: error restoring data";
const PSCCHAR s2010[] = "RUNTIME ERROR: invalid array element size";
const PSCCHAR s2011[] = "RUNTIME ERROR: invalid struct size";
const PSCCHAR s2012[] = "RUNTIME ERROR: modulo zero, result undefined";
const PSCCHAR s2013[] = "RUNTIME ERROR: arrays of this type not supported";
const PSCCHAR s2014[] = "RUNTIME ERROR: out of memory";
const PSCCHAR s2015[] = "RUNTIME ERROR: unsupported typecast";
const PSCCHAR s2016[] = "RUNTIME ERROR: CPU exception. See exception buffer for details.";

/* ---- ATTENTION: always declare FB error messages non-const and use suffix ("Group: , ID:   ") for filling at run-time  --- */
PSCCHAR s3001[]       = "FIRMWARE: general error. Group: , ID:       ";
PSCCHAR s3002[]       = "FIRMWARE: called FB not available. Group: , ID:       ";
PSCCHAR s3003[]       = "FIRMWARE: error accessing the hardware. Group: , ID:       ";

const tPscErrTabEntry IpErrTab_l[] =
    {
        { kIpDivisionByZero,                2001L,  s2001 },
        { kIpArrayIndexInvalid,             2002L,  s2002 },
        { kIpOpcodeInvalid,                 2003L,  s2003 },
        { kIpOpcodeNotSupported,            2004L,  s2004 },
        { kIpExtensionInvalid,              2005L,  s2005 },
        { kIpTaskCmdInvalid,                2006L,  s2006 },
        { kIpPflowNotAvailable,             2007L,  s2007 },
        { kIpInvalidBitRef,                 2008L,  s2008 },
        { kIpErrorRestoreData,              2009L,  s2009 },
        { kIpNoValidArrElementSize,         2010L,  s2010 },
        { kIpInvalidStructSize,             2011L,  s2011 },
        { kIpModuloZero,                    2012L,  s2012 },
        { kIpArrElemNotSupported,           2013L,  s2013 },
        { kIpNoMem,                         2014L,  s2014 },
        { kIpInvalidTypecast,               2015L,  s2015 },
        { kIpCPUException,                  2016L,  s2016 }
};

const tPscErrTabEntry PscErrTab_l[] =
    {
        { kPscSuccess,                         0L,     s0 },
        { kPscGeneralError,                    1L,     s1 },
        { kPscModeConflict,                 1001L,  s1001 },
        { kPscNoMem,                        1002L,  s1002 },
        { kPscInvalidPgm,                   1004L,  s1004 },
        { kPscDwnldError,                   1005L,  s1005 },
        { kPscConfigError,                  1006L,  s1006 },
        { kPscInvalidPgmNr,                 1008L,  s1008 },
        { kPscInvalidSegNr,                 1009L,  s1009 },
        { kPscInvalidSegType,               1010L,  s1010 },
        { kPscSegDuplicate,                 1011L,  s1011 },
        { kPscNoWatchTabEntry,              1012L,  s1012 },
        { kPscUnknownCmd,                   1013L,  s1013 },
        { kPscModeErr,                      1014L,  s1014 },
        { kPscNetError,                     1015L,  s1015 },
        { kPscNetRecSizeError,              1016L,  s1016 },
        { kPscIpExecError,                  1020L,  s1020 },
        { kPscNcExecError,                  1021L,  s1021 },
        { kPscNotValidInRunState,           1025L,  s1025 },
        { kPscNetErrorLastSession,          1103L,  s1103 },
        { kPscUplErrorNotEnabled,           1104L,  s1104 },
        { kPscHistNoFreeEntry,              1105L,  s1105 },
        { kPscHistInvalidID,                1106L,  s1106 },
        { kPscMemoryCorrupted,              1107L,  s1107 },
        { kPscRawFileWriteError,            1201L,  s1201 },
        { kPscRawFileReadError,             1202L,  s1202 },
        { kPscRawFileDeleteError,           1203L,  s1203 },
        { kPscNoBreakpointError,            1601L,  s1601 },
        { kPscMaxBreakpointsError,          1602L,  s1602 },
        { kPscBreakpointNotFoundError,      1603L,  s1603 },
        { kPscDwlTDTError,                  1604L,  s1604 },
        { kPscMoveSegmentError,             1605L,  s1605 },
        { kPscDwlNoLinkerTableError,        1606L,  s1606 },
        { kPscDwlAlignmentError,            1607L,  s1607 },
        { kPscDwlDSSizeError,               1608L,  s1608 },
        { kPscDwlReadSegAddrError,          1609L,  s1609 },
        { kPscDwlResourceReplaceError,      1610L,  s1610 },
        { kPscDwlNoSegTabError,             1611L,  s1611 },
        { kPscDwlProcDataError,             1612L,  s1612 },
        { kPscDwlNoCopyTableError,          1613L,  s1613 },
        { kPscHistMaxHistError,             1614L,  s1614 },
        { kPscHistSizeError,                1615L,  s1615 },
        { kPscHistMutexError,               1616L,  s1616 },
        { kPscHistMaxHistSettingError,      1617L,  s1617 },
        { kPscForceTypeError,               1618L,  s1618 },
        { kPscWatchTypeError,               1619L,  s1619 },
        { kPscWatchDeleteError,             1620L,  s1620 },
        { kPscProcImgError,                 1623L,  s1623 },
        { kPscLoginStatusError,             1624L,  s1624 },
        { kPscLogoutStatusError,            1625L,  s1625 },
        { kPscWriteSegAddrError,            1626L,  s1626 },
        { kPscSaveTempSegError,             1627L,  s1627 },
        { kPscIStackError,                  1630L,  s1630 },
        { kPscIStackError2,                 1631L,  s1631 },

        /* 20070313RVa: new error codes for persistency */
        { kPscPersCRCFailed,                1632L,    s1632 },
        { kPscPersVersionMismatch,          1633L,    s1633 },
        { kPscPersSaveError,                1634L,    s1634 },

        /* error code for data consistency buffer management */
        { kPscDCBufferError,                1635L,    s1635 },

        /* error code new error code for fatal cycle errors */
        { kPscFatalCycleError,              1636L,    s1636 },

        /* error code for no hardware configuration segment */
        { kPscNoHWConfig,                   1637L,    s1637 },

        /* error codes for BL code */
        { kPscBLCodeGenError1,              1638L,    s1638 },
        { kPscBLCodeGenError2,              1639L,    s1639 },
        { kPscBLCodeGenError3,              1640L,    s1640 },
        { kPscBLExecError,                  1641L,    s1641 },

        /* error codes for shared memory data consistency */
        { kPscNoSHMConfig,                  1642L,    s1642 },
        { kPscSHMConfigError,               1643L,    s1643 },
        { kPscSHMInitError,                 1644L,    s1644 },
        { kPscSHMInitErrorSlave,            1645L,    s1645 },
        { kPscSHMInitErrorSetInfo,          1646L,    s1646 },
        { kPscSHMDCBufferError,             1647L,    s1647 },
        { kPscSHMDCSizeError,               1648L,    s1648 },
        { kPscSHMChecksumError,             1649L,    s1649 },

        { kPscErrorRecursion,               1650L,    s1650 },

        { kPscHwDetectError,                1651L,    s1651 },
        { kPscHwConfigureError,             1652L,    s1652 },
        { kPscHwMasterError,                1653L,    s1653 },
        { kPscHwConfChecksumError,          1654L,    s1654 },

        { kPscDynRetainTableFull,           1656L,    s1656 }

};

const tPscErrTabEntry StdErr_l[] =
    {
        /* : Entry for unspecified error */
        { kPscGeneralError,             2L,      s2 }
};

// Request error status that occured lastly
PSCBYTE PscInfGetError()
{
    tPscErrTabEntry const* pErrTab = PSCNULL;
    PSCCHAR * pSz;
    static PSCBYTE bBuff[PSC_SIZEOF_DWORD];
    PSCWORD len = 0;
    PSCBYTE bErrorCode;

    PSCTRACE("\n-> PSC: PscInfGetError ");

    bErrorCode = bLastPlcErr_l;//The last time the PLC executed an error flag

    PSCTRACE("(Evaluate ErrCode: '%02X')", (PSCWORD)bErrorCode);

    pErrTab = PscInfGetErrorEntry(bErrorCode);//Search for error string in string tables

    PscMemAbsSetByte(&bBuff[0], kPscErrInf);/* store identifier of error string */
    PscCsvSetExtRetCode ((PSCBYTE *)&bBuff[0],1);

    /* store error number in sendbuffer */
    PscMemAbsSetDword((PSCBYTE *)&bBuff[0], pErrTab->m_dErrNum);
    PscCsvSetExtRetCode ((PSCBYTE *)&bBuff[0],(PSCWORD)(sizeof(bBuff) / PSC_SIZEOF_BYTE));

    if (pErrTab->m_pErrStr)//copy the error string to sendbuffe
    {
        pSz = (PSCCHAR *)pErrTab->m_pErrStr;
        len = strlen(pSz);
        PscCsvSetExtRetCode ((PSCBYTE *)pErrTab->m_pErrStr,(PSCWORD)(len+1));
    }

    PscSysSetStatus (PSCSTAT_PLCERROR, PSCCLR);

    bLastPlcErr_l = kPscSuccess;

    //Stop send the kPscError flag
    RecordErrFlag = PSCFALSE;
    InfErrFlag    = PSCFALSE;

    return kPscSuccess;
}

// Find error code in error description tables
tPscErrTabEntry const* PscInfGetErrorEntry(PSCBYTE bPlcErrCode_p)
{
    tPscErrTabEntry const* pErrTab = PSCNULL;
    PSCBYTE IpErrCode;
    PSCBYTE i;

    if (bPlcErrCode_p == kIpCPUException)
    {
        /* use ErrorTable with IP-errors */
        for (i = 0; i < (sizeof(IpErrTab_l) / sizeof(tPscErrTabEntry)); i++)
        {
            if (IpErrTab_l[i].m_bErrCode == bPlcErrCode_p)
            {
                pErrTab = &IpErrTab_l[i];
                break;
            }
        }
    }
    else if ((bPlcErrCode_p != kPscIpExecError) && (bPlcErrCode_p != kPscNcExecError) && (bPlcErrCode_p != kPscBLExecError))
    {
        /* PSC-Error */
        for (i = 0; i < (sizeof(PscErrTab_l) / sizeof(tPscErrTabEntry)); i++)
        {
            if (PscErrTab_l[i].m_bErrCode == bPlcErrCode_p)
            {
                pErrTab = &PscErrTab_l[i];
                break;
            }
        }
        /* Inserted new function call */
        if (pErrTab == PSCNULL)
        {
            pErrTab = PscEnvGetOemPscError(bPlcErrCode_p);
        }
        /*---------------------------------------------------------*/
    }
    else
    {
        /* IP-Error or Firmware-FB-Error */
        IpErrCode = PscIpGetIpErrCode();

        /* "real" IP-Error or error code of Firmware-FB ? */
        if (IpErrCode != kIpFirmwareExecError)
        {
            /* "real" IP-Error, thus use of ErrorTable with IP-errors */
            for (i = 0; i < (sizeof(IpErrTab_l) / sizeof(tPscErrTabEntry)); i++)
            {
                if (IpErrTab_l[i].m_bErrCode == IpErrCode)
                {
                    pErrTab = &IpErrTab_l[i];
                    break;
                }
            }
        }
        else
        {

        }
    }

    /* if error code not in ErrorTable, use standard message*/
    if (pErrTab == PSCNULL)
    {
        pErrTab = &StdErr_l[0];
    }

    return pErrTab;
}

tPscErrTabEntry const *PscEnvGetOemPscError(PSCBYTE bPscErrorCode_p)
{
    return PSCNULL;
}

// Request for last interpreter error occured
PSCBYTE PscIpGetIpErrCode(void)
{
    /* because of task switches, there is the global error variable used instead of program data structure */
    return Error_g;
}