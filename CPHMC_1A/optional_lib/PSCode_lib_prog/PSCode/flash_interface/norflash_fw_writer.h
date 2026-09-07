#ifndef _SMARTPLC__NORFLASH_FW_WRITER_H_
#define _SMARTPLC__NORFLASH_FW_WRITER_H_

#include <string.h>
#include <sys/stdint.h>


#include "board/board.h"

/*************************************** flash ***************************************/
/************************************************************************************
 * Region Type	               Start Address           End Address	Size
 * SBL          	       0x0000 0000             0x0008 0000	0x08 0000 （512K）
 * TIFS                        0x0008 0000             0x0010 0000	0x08 0000 （512K）
 * tbl                         0x0010 0000             0x0018 0000	0x08 0000 （512K）
 * atf_spl                     0x0018 0000             0x0028 0000	0x10 0000 （1MB）
 * Core0_App_Image             0x0028 0000             0x0038 0000	0x10 0000 （1MB）
 * Core1_App_Image             0x0038 0000             0x0048 0000	0x10 0000 （1MB）
 * Core2_App_Image             0x0048 0000             0x0058 0000	0x10 0000 （1MB）
 * Core3_App_Image             0x0058 0000             0x0068 0000	0x10 0000 （1MB）
 * Core4_App_Image             0x0068 0000             0x0078 0000	0x10 0000 （1MB）
 * Core5_App_Image             0x0078 0000             0x0088 0000	0x10 0000 （1MB）
 * Core6_App_Image             0x0088 0000             0x0098 0000	0x10 0000 （1MB）
 * Core7_App_Image             0x0098 0000             0x00A8 0000  0x10 0000 （1MB）
 * backup                      0x00A8 0000             0x00C8 0000	0x20 0000 （2MB）
 * **********************************************************************************
 */


#define Core0_APP_ADDR         (0x00280000)
#define Core1_APP_ADDR         (0x00380000)
#define Core2_APP_ADDR         (0x00480000)
#define Core3_APP_ADDR         (0x00580000)
#define Core4_APP_ADDR         (0x00680000)
#define Core5_APP_ADDR         (0x00780000)
#define Core6_APP_ADDR         (0x00880000)
#define Core7_App_ADDR         (0x00980000)


#define FW_MAX_FILE_SIZE       (0x600000) /* 6MB */
#define DEBUG_SIZE             (0x10000)  /* 64KB */

#define MUL_FIRMWARE 1

extern uint8_t FW_BootFlag;

void FW_writeDDR(const void *pBuf, int size, int lastFlag);

#endif  // _SMARTPLC__NORFLASH_FW_WRITER_H_
