/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       file_system.h
 *@author     jinyangh
 *@date       2025.05.21
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.05.21  1.0       jinyangh   Provide the file system interface.
 ******************************************************************************/
#ifndef _FILE_SYSTEM_H
#define _FILE_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include "platform.h"
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define FILE_MAX_NUM	       (32)
#define FILE_NAME_LEN	       (128)
#define FILE_APP_TYPE_LEN      (32)
#define FILE_TIME_LEN	       (7)

//bit0:1-打开文件       0-关闭文件
//bit1:1-读模式打开     0-写模式打开
#define FAT_ITEM_CLOSE         (0x00) //FAT文件表项关闭标志
#define FAT_ITEM_R_OPEN        (0x03) //FAT文件表项读模式打开标志
#define FAT_ITEM_W_OPEN        (0x01) //FAT文件表项写模式打开标志

#define FAT_MODE_R_OPEN        (0x11) //FAT文件表项打开模式：只读
#define FAT_MODE_W_OPEN        (0x22) //FAT文件表项打开模式：只写

#define FLASH_PRO_CPU_IMAGE    (0x11) //CPU程序FLASH0
#define FLASH_PRO_CPU_DATA     (0x12) //CPU数据FLASH1
#define FLASH_PRO_FPGA_IMAGE   (0x21) //FPGA程序FLASH2

#define DEBUG_FILE_SYSTEM       0
/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct
{
    char     Name[FILE_NAME_LEN]; // 应用属性名称
    uint16_t Sector_ID;		      // 存储空间起始扇区号，sector size = 4KB
                                  // （相对于本Core在flashx的sector序号：比如本Core分配了5个扇区，那么Sector_Id为0-4，但其实这5个扇区对应的是flashx的第100-104扇区）
    uint16_t Sector_Num;	      // 存储空间扇区数
    uint16_t Flash_Pro_Seq_Num;	  // 此文件在flash属性的序号
    uint16_t Flash_Pro; 	      // Flash属性: 0x11-cpu程序FLASH0 0x12—cpu数据FLASH1  0x21—FPGA的FLASH2
} EACH_FLASHFAT_STRUCT;// FAT表的表项

typedef struct
{
    uint32_t	         Fat_flag;		         // 固定为0x87654321
    char                 AppType[FILE_APP_TYPE_LEN]; // 装置应用标识，不同应用可设置自己的关键字
    uint16_t 		 FileNum;                    // 有效文件个数
    EACH_FLASHFAT_STRUCT File_Fat[FILE_MAX_NUM];
    uint32_t	         Fat_sum;		         // 整个Fat表校验和，4Byte对齐
} FILE_FAT_TABLE_STRUCT;// 整个FAT表

typedef struct
{
    uint8_t  Use_Flag;            // 此info位置是否已经有文件信息被存储了，当此标志位为0xA5的时候，才能证明此位置存储的有info信息
    char     Name[FILE_NAME_LEN]; // 文件名
    uint8_t  Time[FILE_TIME_LEN]; // 文件下载时间
    uint32_t Size;                // 文件大小，字节为单位
    uint32_t File_Sum;            // 下位机自己算的32bit文件的校验和
    uint32_t File_Src_Sum;        // 上位机下发的文件32bit校验和
    uint32_t Check_Sum;           // 此结构体的32bit校验和，目的在于判断读出的info信息是否等于写入的info信息
} FILE_INFO;// 每个文件的信息，一个fileinfo放一个扇区，因为方便擦除
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
extern Flash_Handle          filesystem_flash_handle;
extern FILE_FAT_TABLE_STRUCT File_Fat_Table;
extern char fatfile_keyword[FILE_KEY_NUM][FILE_NAME_LEN];
extern char fatfile_type[FILE_KEY_NUM][FILE_NAME_LEN];

void     file_system_init(void);
uint32_t check_sum_32bit(uint8_t *ptr, uint32_t size);
uint8_t  compare_filename(const char *p_filename, const char *p_fatitemname, uint16_t Flash_Pro);
void     fpga_flash_switch(uint8_t option);
uint8_t  select_flash(int32_t fileHandle);
uint8_t  get_absolute_sectorID_offset(int32_t fileHandle, uint32_t *sectorID, uint32_t *offset);
int32_t  flash_offsettosector(uint32_t offset, uint32_t *sector);
int32_t  flash_erase_sector(uint32_t absolute_offset);
uint8_t  file_info_write(int32_t fileHandle, FILE_INFO *info);
uint8_t  file_info_read(int32_t fileHandle, FILE_INFO *info);
int32_t  file_open(const char *p_filename, uint8_t mode);
int32_t  file_write(uint8_t *ptr, uint32_t offset, uint32_t size_of_file, int32_t fileHandle);
int32_t  file_read(uint8_t *ptr, uint32_t offset, uint32_t size_of_file, int32_t fileHandle);
int32_t  file_close(int32_t fileHandle, uint32_t file_src_sum);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _FILE_SYSTEM_H */