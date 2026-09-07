/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscTypes.h
 *@author     jinyangh
 *@date       2025.01.02
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.01.02  1.0       jinyangh    example
 ******************************************************************************/
#ifndef _PSCTYPES_H
#define _PSCTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "pscBase.h"
#include <limits.h>
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define PSCCMDBUFFSIZE     512                     /* receive buffer size for commands */
#define PSCPFLOWBUFFSIZE   2048                    /* buffer size powerflow */
#define RECBUFFSIZE       (128 + PSCPFLOWBUFFSIZE) /* send buffer size for receipts */
#define PSCINVALIDVALUE    0xFFFF                  /* used as error value, if the communcation buffer is too small ((PSCWORD) -1) */

#define SIZE_PASSWORD      36 /* 36 bytes for an MD5 hash value consisting of 16 bytes, converted to a hex string with 32 characters, plus a terminating 0, plus alignment to 4. */
#define PSCMAXWATCH        256
#define PSC_SIZEOF_SETDATA 4  //当前最大数据长度4字节

#define DWORD_MAX ULONG_MAX
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
/* tSksVersion and tPscVersion are replaced by this definition*/
typedef struct
{
    PSCDWORD        m_TimeStamp;        /* timestamp */
} tPlcVersion;

typedef struct
{
    const PSCCHAR* m_pProjectAndVersion;
    const PSCCHAR* m_pResourceName;
} tPscAppLibEntry;

/*! struct to request for version of current resource */
typedef struct
{
    PSCCHAR          m_szPrjName[32]; /* project name */
    PSCCHAR          m_szResName[32]; /* resource name */
    tPlcVersion      m_PlcVer;        /* version number of resource */
    PSCDWORD         m_dwBuildDate;   /* creation date of resource */
    PSCDWORD         m_dwLoadDate;    /* download date of resource */

    PSCCHAR          m_szPlatformName[32];

    PSCCHAR          m_szPasswordDeveloper[SIZE_PASSWORD];
    PSCCHAR          m_szPasswordProject[SIZE_PASSWORD];
    PSCCHAR          m_szPasswordCustomer[SIZE_PASSWORD];

    PSCDWORD         m_dwNumAppLibs; /* will not be uploaded to the programming system */
    tPscAppLibEntry* m_pAppLibInfo;  /* will not be uploaded to the programming system */
} tPscResVersion;

typedef struct
{
    PSCCHAR          m_szPrjName[32]; /* project name */
    PSCCHAR          m_szResName[32]; /* resource name */
    tPlcVersion      m_PlcVer;        /* version number of resource */
    PSCDWORD         m_dwBuildDate;   /* creation date of resource */
    PSCDWORD         m_dwLoadDate;    /* download date of resource */

    PSCCHAR          m_szPlatformName[32];

    PSCCHAR          m_szPasswordDeveloper[SIZE_PASSWORD];
    PSCCHAR          m_szPasswordProject[SIZE_PASSWORD];
    PSCCHAR          m_szPasswordCustomer[SIZE_PASSWORD];

    /* The length of pointers may vary in different CPU codes.
     * To maintain consistency in the length of data structures,
     * placeholder members have been added*/
    PSCDWORD         res[3];
} tPscResVersionTable;

/* struct to administrate the commands of the IDE */
typedef struct
{
    PSCBYTE m_bCommand;     /* current command */
    PSCWORD m_wParamSize;   /* size of parameter list */
    PSCBYTE *m_pParamLst;   /* Pointer to parameter list */
    PSCBOOL m_fSendRec;     /* Flag "send receipt" */
} tPscPSCmd;//用于管理IDE的命令

typedef struct
{
    PSCBYTE bCmdBuff[PSCCMDBUFFSIZE];   //命令输入缓冲区 /* Input buffer for commands */
    PSCBYTE bRecBuff[RECBUFFSIZE];      //接收数据缓冲区，其实是发送出去的数据的缓冲区 /* Sendbuffer for receipts */
    PSCWORD wUsedRecSize;               //接收数据缓冲区用了多少 /* Level of Receiptpuffer */
    PSCWORD wMaxRecSize;                //接收数据缓冲区的最大可能大小 /* max. possible size of receipt */
    PSCBOOL fPrestCmd;                  //是否为内部命令 /* PSCTRUE -> "internal command" */
    PSCBOOL fRecEnabled;                //使能命令输入缓冲区 /* PSCTRUE -> Inputbuffer enabled */
} tPscCmdBuffer;//缓存命令和接收到的数据

/* structure to add a watch item */
typedef struct
{
    PSCBYTE                 m_Type;    //类型 /* Unused/bit/byte (sequence) */
    PSCWORD                 m_Pgm;     //程序编号 /* Program number */
    PSCDWORD                m_Addr;    //数据地址
    PSCWORD                 m_Size;    //字节数 /* Number of bytes */
} tPscDataAdr;//监视项

/* structure to hold watch data for one connection */
typedef struct
{
    PSCBOOL     fWatchEnable;          //是否启用监视 /* watching activ */
    PSCDWORD    dwWatchCom;            //监视列表中的数据对应的端口号（用于区分不同核的监视数据信息）
                                       //此变量放在监视列表而不放在监视项的原因：当前上位机每次只能监视一个核，并且切换监视范围时，监视列表会被清空
    tPscDataAdr WatchLst[PSCMAXWATCH]; //监视列表，由多个监视项（条目）组成 /* table of watch items */
    PSCDWORD    dwWatchEntriesUsed;    //已使用的条目数
    PSCWORD     wCurrWatchId;          //即将要处理的监视项ID /* next watch item to process */
    PSCBOOL     fDataAvailable;        //是否有新数据可用 /* (new) data available */
    PSCBYTE     bWatchMode;            //监视模式（单次/全部）/* watch modes (Single/All) */
    PSCBOOL     fDataRequestsLocked;   //数据请求锁 /* data requests locked */
} tPscWatchTable;//监视列表管理器

/* struct to set one variable */
typedef struct
{
    PSCBYTE   m_Type;                       //类型/* Unused/bit/byte (sequence) */
    PSCWORD   m_Pgm;                        //程序编号/* Program number */
    PSCDWORD  m_Addr;                       //数据地址
    PSCWORD   m_Size;                       //要设置的字节数/* Number of bytes to be set */
    PSCBYTE   m_fMakePersistent;            //持续存储新值并在重新启动时再次应用它
    PSCBYTE   m_Data[PSC_SIZEOF_SETDATA];   //存放要设置的数据
} tPscSetSingleData;//设置一个变量值

typedef struct
{
    PSCBOOL           SetVariableFlag;
    PSCDWORD          dwSetCom;           //设置列表中的数据对应的端口号（用于区分不同核的设置数据信息）
                                          //此变量放在设置列表而不放在设置项的原因：当前上位机每次只能设置一个核的数据
    tPscSetSingleData SetData;
} tPscSetData;

/* struct to force a variable */
typedef struct
{
    PSCBYTE  m_Type;                      /* Unused/bit/byte (sequence) */
    PSCWORD  m_Pgm;                       /* Program number */
    PSCDWORD m_Addr;
    PSCWORD  m_Size;                      /* Number of bytes */
    PSCBYTE  m_Data[PSC_SIZEOF_SETDATA];
} tPscForceItem;//强制变量

typedef struct
{
    PSCDWORD          dwForceCom;         //强制列表中的数据对应的端口号（用于区分不同核的强制数据信息）
                                          //此变量放在强制列表而不放在强制项的原因：当前上位机每次只能强制一个核的数据
    PSCDWORD          dwForceEntriesUsed; //已使用的条目数
    tPscForceItem     ForceLst[PSCMAXWATCH];
} tPscForceTable;//强制变量管理器

typedef struct
{
    PSCBYTE  m_Type; /* Unused/bit/byte (sequence) */
    PSCWORD  m_Pgm;  /* Program number */
    PSCDWORD m_Addr;
} tPscUnForceItem;//删除强制变量

typedef struct
{
    PSCBYTE        m_bErrCode;
    PSCDWORD       m_dErrNum;
    const PSCCHAR *m_pErrStr;
} tPscErrTabEntry;//错误信息管理器

/* struct to watch and set varaiables */
typedef enum
{
    kPscAccessUndef       = 0x00, /* Watch entry unused */
    kPscAccessBitByAdr    = 0x01, /* Watch or set bit */
    kPscAccessBitByRef    = 0x02, /* Watch or set bit */
    kPscAccessByteByAdr   = 0x03, /* Watch or set byte (sequence) */
    kPscAccessByteByRef   = 0x04, /* Watch or set byte (sequence) */
    kPscAccessByteByWave  = 0x10, /* Watch Wave */
} tPscAccessType;

/* Definition of watch mode */
typedef enum
{
    kPscWatchSingle = 0x00, /* only one variable/cycle */
    kPscWatchAll    = 0x01  /* all variables/cycle */
} tPscWatchMode;//监视模式

/* inserted for function <PscCsvPresetCmd> */
/* Parameter to enable/disable external communicaton */
/* of function <PscCsvPresetCmd> */
typedef enum
{
    kPscEnableExtrnComm  = 0x00,//启用外部通信
    kPscDisableExtrnComm = 0xFF //禁用外部通信
} tPscPresetCmdMode;

typedef enum
{
    kPscRawFileFirmware = 0,
    kPscRawFileFPGAData = 1
} tRawFileTypes;//下载文件的类型

/* parameters for control command <SetState> */
typedef enum
{
    kPscCtrStop        = 0x00, /* Command "Stop" */
    kPscCtrColdStart   = 0x01, /* Command "coldstart" */
    kPscCtrWarmStart   = 0x02, /* Command "Warmstart" */
    kPscCtrHotStart    = 0x03, /* Command "continue" */
    kPscCtrSingleCycle = 0x04, /* Command "SingleCycle" */
    kPscCtrClearSystem = 0x05, /* Command "ClearSystem" */
    kPscCtrRestoreDS   = 0xFF  /* Command "Restore DS" */
} tPscCtrLst;

/* Definition of error codes */
typedef enum
{
    kPscSuccess                 = 0x00, /* everything OK */

    /* INTERNAL ERRORS - 0xC0..0xFF */
    kPscGeneralError            = 0xFF, /* general error */
    kPscUnknownCmd              = 0xFE, /* invalid control command (sent by OpenPCS) */
    kPscModeErr                 = 0xFD, /* operation not allowed in current mode */
    kPscNoMem                   = 0xFC, /* out of memory */
    kPscNetError                = 0xFB, /* general network error */
    kPscNetRecSizeError         = 0xFA, //接收缓存大小错误/* network: accepted receipt shipment too small */
    kPscNoWatchTabEntry         = 0xF9, /* no free watch ID available */
    kPscModeConflict            = 0xF8, /* mode conflict (command not allowed in current mode - run/stop switch) */
    kPscNetErrorLastSession     = 0xF7, /* network error (last connection) */
    kPscInvalidPgm              = 0xF6, /* no valid program available */
    kPscInvalidPgmNr            = 0xF5, /* program number invalid */
    kPscInvalidSegNr            = 0xF4, /* segment number invalid */
    kPscInvalidSegType          = 0xF3, /* segment type invalid */
    kPscSegDuplicate            = 0xF2, /* segment exists already  */
    kPscDwnldError              = 0xF1, /* download incomlete/logical error */
    kPscConfigError             = 0xF0, /* configuration error/wrong program */
    kPscUplErrorNotEnabled      = 0xEF, /* segment not found on upload */
    kPscMemoryCorrupted         = 0xEE, /* pointers in IECMEMORY out of the valid range */
    kPscIpExecError             = 0xED, /* error at call of interpreter */
    kPscNcExecError             = 0xEC, /* error at execution of native code */
    kPscInvalidFilePath         = 0xEB, /* invalid file path when downloading raw files */
    kPscNotValidInRunState      = 0xEA, /* action not valid in running state of PSC */
    kPscHistNoFreeEntry         = 0xE9, /* no free hist table entry available */
    kPscHistInvalidID           = 0xE8, /* invalid hist table ID on delete or get */
    kPscNoBreakpointError       = 0xE7,
    kPscMaxBreakpointsError     = 0xE6,
    kPscBreakpointNotFoundError = 0xE5,
    kPscDwlTDTError             = 0xE4,
    kPscMoveSegmentError        = 0xE3,
    kPscDwlNoLinkerTableError   = 0xE2,
    kPscDwlAlignmentError       = 0xE1,
    kPscDwlDSSizeError          = 0xE0,
    kPscDwlReadSegAddrError     = 0xDF,
    kPscDwlResourceReplaceError = 0xDE,
    kPscDwlNoSegTabError        = 0xDD,
    kPscDwlProcDataError        = 0xDC,
    kPscDwlNoCopyTableError     = 0xDB,
    kPscHistMaxHistError        = 0xDA,
    kPscHistSizeError           = 0xD9,
    kPscHistMutexError          = 0xD8,
    kPscHistMaxHistSettingError = 0xD7,
    kPscRawFileWriteError       = 0xD6, /* writing of raw file failed (disk full, write protected, etc.) */
    kPscRawFileReadError        = 0xD5, /* reading of raw file failed (file does not exist, no permission, etc.) */
    kPscRawFileDeleteError      = 0xD4, /* error deleting raw file */
    kPscForceTypeError          = 0xD3,
    kPscWatchTypeError          = 0xD2,
    kPscWatchDeleteError        = 0xD1,
    kPscProcImgError            = 0xD0,
    kPscLoginStatusError        = 0xCF,
    kPscLogoutStatusError       = 0xCE,
    kPscWriteSegAddrError       = 0xCD,
    kPscSaveTempSegError        = 0xCC,
    kPscIStackError             = 0xCB,
    kPscIStackError2            = 0xCA,
    kPscPersCRCFailed           = 0xC9,
    kPscPersVersionMismatch     = 0xC8,
    kPscPersSaveError           = 0xC7,

    /* error code for data consistency buffer management */
    kPscDCBufferError           = 0xC6, /* error getting write buffer */

    /* error code for no hardware configuration segment */
    kPscNoHWConfig              = 0xC5,

    /* error codes for BL code */
    kPscBLCodeGenError1         = 0xC4, /* unsupported UCode instruction */
    kPscBLCodeGenError2         = 0xC3, /* invalid FB call */
    kPscBLCodeGenError3         = 0xC2, /* unsupported combination of UCode instructions */
    kPscBLExecError             = 0xC1, /* runtime error during execution */

    /* error codes for shared memory data consistency */
    kPscNoSHMConfig             = 0xC0, /* configuration segment not found */

    /* INTERNAL ERRORS 2 - 0xA0..0xBF */
    kPscSHMConfigError          = 0xBF, /* error in configuration, e.g. segments not found, ... */
    kPscSHMInitError            = 0xBE, /* error during creation or initialization of the memory (master) */
    kPscSHMInitErrorSlave       = 0xBD, /* error connecting to the memory (slave) */
    kPscSHMInitErrorSetInfo     = 0xBC, /* error setting up CPU-local information */
    kPscSHMDCBufferError        = 0xBB, /* error getting write buffer */
    kPscSHMDCSizeError          = 0xBA, /* size of shared memory is too small */
    kPscSHMChecksumError        = 0xB9, /* shared memory configuration from master and slave(s) is different */
    kPscSHMDCLCError            = 0xB8, /* error when locking read buffer: LC was changed by the writer (this error is handled by retry) */

    kPscInitModeExecError       = 0xAF, /* error when executing init mode */
    kPscErrorRecursion          = 0xAE, /* recursion in user FBs */
    kPscHwConfChecksumError     = 0xAD, /* hardware configuration from master and slave(s) is different */

    kPscRawFileChecksumError    = 0xAC, /* raw file received with checksum mismatch */

    kPscDynRetainTableFull		= 0xA0, /* no more free entry in the dynamic retain table */

    /* until (down to) 0xA0: reserved for future internal error codes */

    /* RUNTIME ERRORS - 0x80..0x9F - defined in tIpErrorCode, ip_def.h */

    /* COMMUNICATION ERRORS - 0x70..0x7F */

    /* USER ERRORS have a separate number range, only defined on user application level */

    /* HARDWARE MONITORING ERRORS - 0x60..0x6F */

    /* TASK ADMINISTRATION ERRORS - 0x50..0x5F */
    kPscFatalCycleError         = 0x5F, /* fatal cycle error (task cycle length exceeded in <n> consecutive cycles) */
    kPscSingleCycleError        = 0x5E, /* single cycle error (task cycle length exceeded once) */

    /* INITIAL ERRORS - 0x40..0x4F */
    kPscInitMemoryError          = 0x4F, /* application memory not allocated */
    kPscInitSHMError             = 0x4E, /* shared memory not allocated/connected */
    kPscInitCommError            = 0x4D, /* communication not initialized */
    kPscInitTasksError           = 0x4C, /* tasks and/or background services not started */
    kPscInitErrLogError          = 0x4B, /* error log not initialized */
    kPscInitRestoreError         = 0x4A, /* persistent application not restored */
    kPscInitExceptionBufferError = 0x49, /* exception buffer not initialized */
    kPscHwDetectError            = 0x48, /* error during hardware detection */
    kPscHwConfigureError         = 0x47, /* error during apply hardware configuration settings */
    kPscHwMasterError            = 0x46  /* error on the master during hardware check and configuration */
} tPscErrorCode;

/* control commands of the IDE for the PLC */
typedef enum
{
    kPscCmdLogin                          = 0x01, /* command "Login" */
    kPscCmdLogout                         = 0x02, /* command "Logout" */
    kPscCmdReset                          = 0x03, /* command "Reset" */
    kPscCmdDwlResource                    = 0x04, /* command "Download Resource" */
    kPscCmdDwlProgram                     = 0x05, /* command "Download Program" */
    kPscCmdDwlSegment                     = 0x06, /* command "Download Segment" */
    kPscCmdSetState                       = 0x07, /* command "Set State" */
    kPscCmdGetResVer                      = 0x08, /* command "Get Resource Version" */
    kPscCmdGetPlcVer                      = 0x09, /* command "Get Firmware Version" */
    kPscCmdDiscradWatch                   = 0x0A, /* command "Discard Watch Table" */
    kPscCmdEnableWatch                    = 0x0B, /* command "Enable Watching" */
    kPscCmdAddWatchInstr                  = 0x0C, /* command "Add Watch Instruction" */
    kPscCmdSetVariable                    = 0x0D, /* command "Set Variable" */
    kPscCmdGetErrorInf                    = 0x0E, /* command "Get Error Information" */
    kPscCmdUplGetSegInf                   = 0x0F, /* command "Get Segment Information" */
    kPscCmdUplGetSegData                  = 0x10, /* command "Get Segment Data" */
    kPscCmdStartPflow                     = 0x11, /* command "Start Powerflow" */
    kPscCmdStopPflow                      = 0x12, /* command "Stop  Powerflow" */
    kPscCmdDwlSingleSegment               = 0x13, /* command   "download configuration data"*/
    kPscCmdForceVariable                  = 0x14, /* command "Force Variable" */
    kPscCmdDisableForceVariable           = 0x15, /* command "Force Variable" */
    kPscCmdDelWatchInstr                  = 0x16, /* command "Del Watch Instruction" */
    kPscCmdAddMulWatchInstr               = 0x17, /* command "Add Multiple Watch Instruction" */
    kPscCmdDwlMultiSegment                = 0x18, /* download multiple segments at once */
    kPscCmdGetLibInfo                     = 0x19, /* get firmware library information */

    kPscCmdRequestData                    = 0x20, /* command "Request Data" (to Watch) */
    kPscCmdDwlWithoutStop                 = 0x21, /* command "Download data (Resource/Program/Segments) while PLC is running */

    kPscCmdHistAddElem                    = 0x22, /* command "Variable History - add element"        */
    kPscCmdHistDelElem                    = 0x23, /* command "Variable History - delete element"    */
    kPscCmdHistGetRange                   = 0x24, /* command "Variable History - get data"    */
    kPscCmdHistGetRangeCont               = 0x25, /* command "Variable History - get data", input data */
    kPscCmdHistGetRangeResult             = 0x26, /* command "Variable History - get data, result data" */
    kPscCmdHistClear                      = 0x27, /* command "Variable History - clear" */
    kPscCmdHistUpdate                     = 0x28, /* command "Variable History - update" */
    kPscCmdHistUpdateCont                 = 0x29, /* command "Variable History - update, data part" */

    kPscCmdUplGetSegInfNoError            = 0x2A, /* command "Get Segment Information - return error code as data" */
    kPscCmdSaveSystem                     = 0x2B, /* put Resource to persistent storage */
    kPscCmdCheckResVer                    = 0x2C, /* check resource version */
    kPscCmdDwlRawFile                     = 0x30, /* command "Download RAWFile */
    kPscCmdDwlRawFileSegment              = 0x31, /* command "Download RAW File Segment" for larger RAWFiles*/
    kPscCmdDwlRawFileLastSegment          = 0x32, /* command "Download RAW File Last Segment" for last RAWFile Segment*/
    kPscCmdUplRawFile                     = 0x33, /* command "Upload RAWFile */
    kPscCmdUplRawFileSegment              = 0x34, /* command "Upload RAW File Continue" for larger RAWFiles*/
    kPscCmdUplRawFileLastSegment          = 0x35, /* command "Upload RAW File Last Segment" for last RAWFile Segment*/
    kPscCmdGetRawFileInfo                 = 0x36, /* command "GetRAWFileInfo" to be able to compare the up-to-dateness of the file */
    kPscCmdContDwlRawFileSegment          = 0x37, /* continue "download rawfile segment" */

    kPscCmdDwlDcfFilesFinished            = 0x38, /* command "download binary Dcf segment files finshied" */

    kPscCmdContDwlRes                     = 0x83, /* Continue "Download Resource" */
    kPscCmdContDwlSeg                     = 0x85, /* Continue "Download Segment" */
    kPscCmdContDwlSingleSeg               = 0x86, /* download config data*/

    kPscCmdDwlCompletion                  = 0x87, /* Postprocessing finish download */
    kPscCmdAddMulWatchInstrCont           = 0x88, /* get data for multiple watch instr */
    kPscCmdContDwlMultiSeg                = 0x89, /* get the data for multiple segments*/
    kPscCmdDwlWithoutStopCont             = 0x8A, /* Continue kPscCmdDwlWithoutStop */
    kPscCmdReboot                         = 0x8B, /* reboot PLC */

    kPscCmdRestoreSystem                  = 0x8C, /* restore resource from persistent storage */

    kPscCmdClearErrLog                    = 0x8D, /* clear error log */

    /* Breakpoints */
    kPscCmdBrPtAdd                        = 0xA0, /* Breakpoint Command */
    kPscCmdBrPtRemove                     = 0xA1, /* Breakpoint Command */
    kPscCmdBrPtRemoveAll                  = 0xA2, /* Breakpoint Command */
    kPscCmdBrPtSleepAll                   = 0xA3, /* Breakpoint Command */
    kPscCmdBrPtAwakeAll                   = 0xA4, /* Breakpoint Command */

    kPscCmdBrPtCtrlGo                     = 0xA5, /* Breakpoint Ctrl Command */
    kPscCmdBrPtCtrlStepIn                 = 0xA6, /* Breakpoint Ctrl Command */
    kPscCmdBrPtCtrlStepOut                = 0xA7, /* Breakpoint Ctrl Command */
    kPscCmdBrPtCtrlStepOver               = 0xA8, /* Breakpoint Ctrl Command */

    /* Parameters */
    kPscCmdSetParameter                  = 0xB0, /* Set a Parameter */

    /* Commands with long parameter data, resulting in messages of more than 255 bytes size */
    /* In those messages, the parameter size is given in the WORD at bytes [1] to [2] of the message (treated in PscCsvCheckCmdReceipt()) */
    kPscCmdSetVariableLong               = 0xB1, /* command "Set Variable (long data)" */
    kPscCmdForceVariableLong             = 0xB2, /* command "Force Variable (long data)" */
    kPscCmdMultiSetVariable              = 0xB3, /* command set multiple variable */
    kPscCmdMultiSetVariableCont          = 0xB4, /* continue command set multiple variable */

    /* OEM functions */
    kPscCmdOEMSendCmd                    = 0xC8, /* OEM Command */

    /* Functions for 32Bit online server */
    kPscCmdTerminate                     = 0xF0, /* Terminate Signal */
    kPscDownloadResourceCollection       = 0xF1, /* Resource Collection for download */
    kPscNewAkkuStatusCollection          = 0xF2, /* New Akku Status download */
    kPscNewVariableStatusCollection      = 0xF3, /* New Variable Status download */
    kPscLoginCollection                  = 0xF4, /* Login Collection (login and Resourceinformation */
    kPscSetSleepTime                     = 0xF5, /* Change the sleeptime of the queue */
    kPscDownloadResourceUpdateCollection = 0xF6, /* Collection for Update download */
    kPscCheckResourceVersion             = 0xF7, /* Check the Version of the PLC Version against the PCD */
    kPscDownloadUserSegmentCollection    = 0xF8, /* Download configuration segment. */
    kPscUploadResourceCollection         = 0xF9, /* Upload of a resource on the PLC */
    kPscCompareVersions                  = 0xFA, /* Compare the OemId and the HW-/FW-/Kernel-Versions of the resource and the PLC */
    kPscDownloadResourceIncCollection    = 0xFB, /* Resource Collection for incremental download */
    kPscUploadVarTabCollection           = 0xFC, /* Upload of VarTab segments */
    kPscCompareFwLib                     = 0xFD  /* Compare the firmware libraries of the resource and the PLC */
} tPscCmdLst;

/* Definition of events */
typedef enum
{
    kPscNoChange                 = 0x00, /* no change */
    kPscStateChg                 = 0x01, /* updated status of controller*/
    kPscError                    = 0x02, /* error occured (request GetErrInf) */
    kPscPflowData                = 0x03, /* new powerflow data available */
    /*kPscWarning                  = 0x04,*/ /* Error/Warning (request GetErrInf) */

    kPscWatchID                  = 0x05, /* Watch-ID in receipt buffer */
    kPscWatchData                = 0x06, /* Watch-Data in receipt buffer */
    kPscResVer                   = 0x07, /* ResourceVersion in receipt buffer */
    kPscErrInf                   = 0x09, /* Errorstring in receipt buffer */
    kPscSegInf                   = 0x0A, /* Segment informationen in receipt buffer */

    kPscBreakPointReached        = 0x0B, /* Breakpoint reached */
    kPscBreakPointData           = 0x0C, /* Breakpoint data for signaled breakpoints */
    kPscSysID                    = 0x0D, /* PSC-ID in receipt buffer */
    kPscLockTime                 = 0x0E, /* specification of lock time in receipt buffer */
    kPscDump                     = 0x0F, /* stack,Reg for errors */
    kPscWatchIDList              = 0x10,
    kPscExtCap                   = 0x11, /* extendet capabilities */
    kPscOEMVers                  = 0x12, /* OEM Version info */
    kPscOnlineEditChangesApplied = 0x13, /* old resource has been replaced */
    kPscSaveSystemCmdFinished    = 0x14, /* PscEnvSaveSystemCmd has finished */
    kPscRestoreSystemCmdFinished = 0x15, /* PscEnvRestoreSystemCmd has finished */
    kPscFwLibInfo                = 0x16, /* firmware library info in receipt buffer */
    kPscPlcVer                   = 0x30, /* FirmwareVersion in receipt buffer, Smartsim is 0x08 */

    kPscOEMData                  = 0xA0,

    kPscRemovedWatchID           = 0xA5, /* Watch-ID in receipt buffer */
    kPscRawFileInf               = 0xA6  /* Raw file information in receipt buffer */
} tPscEvent;

/* structure to access the task configuration part of the hardware configuration segment */
typedef struct
{
    PSCDWORD dwT0Interval; /* T0 interval in microseconds */
    PSCDWORD dwT1Factor;   /* T1 multiplication factor */
    PSCDWORD dwT2Factor;   /* T2 multiplication factor */
    PSCDWORD dwT3Factor;   /* T3 multiplication factor */
    PSCDWORD dwT4Factor;   /* T4 multiplication factor */
    PSCDWORD dwT5Factor;   /* T5 multiplication factor */

    PSCDWORD dwI1SamplingTime; /* I1 equivalent sampling time in microseconds */
    PSCDWORD dwI2SamplingTime; /* I2 equivalent sampling time in microseconds */
    PSCDWORD dwI3SamplingTime; /* I3 equivalent sampling time in microseconds */
    PSCDWORD dwI4SamplingTime; /* I4 equivalent sampling time in microseconds */
    PSCDWORD dwI5SamplingTime; /* I5 equivalent sampling time in microseconds */
    PSCDWORD dwI6SamplingTime; /* I6 equivalent sampling time in microseconds */
    PSCDWORD dwI7SamplingTime; /* I7 equivalent sampling time in microseconds */
    PSCDWORD dwI8SamplingTime; /* I8 equivalent sampling time in microseconds */

    PSCBYTE bI1Source; /* I1 source */
    PSCBYTE bI2Source; /* I2 source */
    PSCBYTE bI3Source; /* I3 source */
    PSCBYTE bI4Source; /* I4 source */
    PSCBYTE bI5Source; /* I5 source */
    PSCBYTE bI6Source; /* I6 source */
    PSCBYTE bI7Source; /* I7 source */
    PSCBYTE bI8Source; /* I8 source */
} tTaskConfig;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
/* task settings */
extern tTaskConfig taskConfig_g;
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _PSCTYPES_H */
