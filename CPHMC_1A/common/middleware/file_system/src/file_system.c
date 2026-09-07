/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       file_system.c
 *@author     jinyangh
 *@date       2025.05.21
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.05.21  1.0       jinyangh   Provide the file system interface.
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "file_system.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
uint8_t  FatFile_Flag[FILE_MAX_NUM]  = {0}; //FAT文件表项的标志位
FILE_INFO File_Info = {0};

//以下资源为本核下载文件时的所有文件公用资源
uint8_t  PageBuffer[FLASH_PAGE_SIZE] = {0}; //暂存要写入的数据
#if DEBUG_FILE_SYSTEM
uint8_t  PageBufferBak[FLASH_PAGE_SIZE] = {0}; //暂存要写入的数据
#endif
uint16_t PageBuffer_size    = 0;            //PageBuffer中存储数据的个数，范围0-255，因为PageBuffer_size不会等于256
uint32_t file_offset        = 0;            //下一次PageBuffer数据要写入的偏移
uint32_t file_checksum      = 0;            //文件校验和（4Byte对齐）
uint32_t file_size          = 0;            //文件的大小

int32_t  fileHandle_last    = 0xFF;         //记录最后一个open的文件handle，如果此时没有文件被打开，则为0xFF
uint32_t last_erased_sector = 0xFFFF;       //记录上一次擦除的扇区ID，如果当前扇区与上一次擦除的扇区相同，则不擦除

uint8_t flash_chipSelect = 0;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */
/*
 * 文件系统使用方法：
 * 需求：sbl、app image、spl、fpga image都是只能由R0下载，所以只有flash1向4个R核开放，flash0和flash2都只能被R0使用
 * 注意：同一时间，有且仅有一个文件被打开（不管是只读打开还是只写打开，都只允许一个文件被打开），如果存在打开文件1后，又打开文件2，那么会自动关闭文件1
 * 写：file_open -> file_write -> file_close：必须执行此顺序后，数据才算写入完成Flash
 * 读：file_open -> file_info_read -> file_read -> file_close：先用file_info_read读出文件信息及长度，再根据长度file_read数据
 * 注意：调用file_info_read把文件信息读出后，先比较File_Src_Sum和File_Sum，检查校验和是否一致，如果一致则说明flash此时存的文件是正确的
 * */
/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/**
 * @brief       : 文件系统初始化，用于自动计算FAT表中配置了多少个FAT文件项
 */
void file_system_init(void)
{
    FILE_FAT_TABLE_STRUCT *pfat_tab  = &File_Fat_Table;
    EACH_FLASHFAT_STRUCT  *pfat_item = NULL;

    for (uint16_t index = 0; index < FILE_MAX_NUM; index++)
    {
        pfat_item = &pfat_tab->File_Fat[index];
        if (strcmp(pfat_item->Name, "FAT_END") != 0)
        {
            pfat_tab->FileNum++;
        }
        else
        {
            break;
        }
    }
}

/**
 * @brief       : 计算校验和 (4Byte对齐，不够的补0)
 * @param  ptr  : 要计算校验和的数据指针
 * @param  size : 要计算校验和的数据大小 (Byte为单位)
 * @return      : 32bit校验和
 */
uint32_t check_sum_32bit(uint8_t *ptr, uint32_t size)
{
    uint32_t *temp = (uint32_t *)ptr;
    uint32_t  data = 0;
    uint32_t  sum  = 0;
    uint32_t  num  = size / 4;

    if((ptr != NULL) && (size > 0))
    {
        if(0 != (size % 4))
        {
            // 不够4Byte对齐，则补齐
            memcpy(&data, &ptr[num * 4], size % 4);
        }

        for(uint32_t i = 0; i < num; i++)
        {
            sum += temp[i];
        }
        sum += data;
    }

    return sum;
}

/**
 * @brief               : 比较文件名
 *                        配置文件：比较第一个'.'后面的后缀是否一致，并且比较前缀中是否包含fat表中的前缀名
 *                        其他文件：比较关键字和后缀
 * @param p_filename    : 文件名
 * @param p_fatitemname : FAT表项文件名
 * @param fat_flash_pro : FAT表项flash属性
 * @return              : 0-比较失败，1-比较成功
 */
uint8_t compare_filename(const char *p_filename, const char *p_fatitemname, uint16_t fat_flash_pro)
{
    if((p_filename != NULL) && (p_fatitemname != NULL))
    {
        if (fat_flash_pro == FLASH_PRO_CPU_DATA)
        {
            char *result1 = NULL;
            char *result2 = NULL;
            char  ch      = '.';

            result1 = strchr(p_filename, ch);
            result2 = strchr(p_fatitemname, ch);
            if (result1 != NULL && result2 != NULL) //找到了'.'字符在两个字符串中第一次出现的位置
            {
                // 比较p_filename和p_fatitemname第一次出现'.'后面的字符（比较后缀）
                if (strcmp(result1, result2) != 0)
                {
                    return 0; //文件扩展码不一致
                }

                char key_word[128]; //用于存放p_fatitemname后缀前的字符串：例如p_fatitemname是g1.cfg，那么key_word就是g1
                memcpy(key_word, p_fatitemname, strlen(p_fatitemname) - strlen(result2));
                key_word[strlen(p_fatitemname) - strlen(result2)] = '\0';
                // 比较文件扩展名前面，p_filename中是否包含p_fatitemname
                if (strstr(p_filename, key_word) != NULL)
                {
                    return 1;
                }
            }
        }
        else if ((fat_flash_pro == FLASH_PRO_CPU_IMAGE) || (fat_flash_pro == FLASH_PRO_FPGA_IMAGE))
        {
            // 不是配置文件，就对比关键字和文件类型，如果p_fatitemname和p_filename中有同样的关键字和文件类型，则认为找到目标文件
            for (uint8_t i = 0; i < FILE_KEY_NUM; i++)
            {
                if ((strstr(p_fatitemname, fatfile_keyword[i]) != NULL) &&
                    (strstr(p_fatitemname, fatfile_type[i]) != NULL) &&
                    (strstr(p_filename, fatfile_keyword[i]) != NULL) &&
                    (strstr(p_filename, fatfile_type[i]) != NULL))
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

/**
 * @brief        : 启用CPU控制FPGA的FLASH的开关
 * @param option : 1：CPU启用FPGA的FLASH，0：CPU禁用FPGA的FLASH
 */
void fpga_flash_switch(uint8_t option)
{
    if(option == 1)
    {
        //选择多路模拟开关，让FPGA的Flash切换到CPU侧
        GPIO_pinWriteLow_com (FPGA_FLASH_GPIO_BASS_A, FPGA_FLASH_GPIO_PIN_A);
        GPIO_pinWriteHigh_com(FPGA_FLASH_GPIO_BASS_B, FPGA_FLASH_GPIO_PIN_B);
    }
    else
    {
        //选择多路模拟开关，让FPGA的Flash切换到FPGA侧
        GPIO_pinWriteHigh_com(FPGA_FLASH_GPIO_BASS_A, FPGA_FLASH_GPIO_PIN_A);
        GPIO_pinWriteHigh_com(FPGA_FLASH_GPIO_BASS_B, FPGA_FLASH_GPIO_PIN_B);
    }
}

/**
 * @brief            : 根据FAT表项中的FLASH属性切换片选
 * @param fileHandle : 文件句柄，也就是FAT表索引
 * @return           : flash片选
 */
uint8_t select_flash(int32_t fileHandle)
{
    if(fileHandle < 0 || fileHandle >= FILE_MAX_NUM)
    {
        return 0xFF;
    }

    switch(File_Fat_Table.File_Fat[fileHandle].Flash_Pro)
    {
    case FLASH_PRO_CPU_IMAGE:
        filesystem_flash_handle = flash_handle0;
        return FLASH_0;
    case FLASH_PRO_CPU_DATA:
        filesystem_flash_handle = flash_handle1;
        return FLASH_1;
    case FLASH_PRO_FPGA_IMAGE:
        fpga_flash_switch(1);
        filesystem_flash_handle = flash_handle2;
        return FLASH_2;
    default:
        return 0xFF;
    }
}

/**
 * @brief            : 获取FAT文件表项对应的Flash绝对基扇区号和Flash绝对偏移基地址 (获取本Core在flashx的绝对偏移基地址和扇区ID)
 * @param fileHandle : 文件句柄，也就是FAT表索引
 * @param sectorID   : Flash绝对基扇区号
 * @param offset     : Flash绝对偏移基地址
 * @return           : 0-获取失败，1-获取成功
 */
uint8_t get_absolute_sectorID_offset(int32_t fileHandle, uint32_t *sectorID, uint32_t *offset)
{
    if(fileHandle < 0 || fileHandle >= FILE_MAX_NUM)
    {
        return 0;
    }

    switch(File_Fat_Table.File_Fat[fileHandle].Flash_Pro)
    {
    case FLASH_PRO_CPU_IMAGE:
        *offset = APP_IMAGE_OFFSET_BASE;
        break ;
    case FLASH_PRO_CPU_DATA:
        *offset = CONFIG_FILE_OFFSET_BASE;
        break ;
    case FLASH_PRO_FPGA_IMAGE:
        *offset = FPGA_IMAGE_OFFSET_BASE;
        break ;
    default:
        return 0;
    }

    flash_offsettosector(*offset,sectorID);

    return 1;
}

/**
 * @brief        : 根据绝对地址获取扇区ID
 * @param offset : 绝对地址
 * @param sector : 扇区ID
 * @return       : 0-获取成功，-1-获取失败
 */
int32_t flash_offsettosector(uint32_t offset, uint32_t *sector)
{
    int32_t status = SystemP_FAILURE;

    if(sector != NULL)
    {
        uint32_t sectorSize  = 4096;
        uint32_t sectorCount = 4096;

        status   = SystemP_SUCCESS;

        *sector  = offset   / sectorSize;
        if (*sector >= sectorCount)
        {
            /* beyond limits for this flash */
            status = SystemP_FAILURE;
        }
    }

    return status;
}

/**
 * @brief                 : 擦除一个扇区
 * @param absolute_offset : 绝对地址
 * @return                : -1-擦除失败或操作错误，0-不需要擦除或擦除成功
 * @attention             : 此函数会根据绝对地址来判断此扇区是否已经擦除，如果在当前open的文件中已经擦除了，则不需要再擦除
 */
int32_t flash_erase_sector(uint32_t absolute_offset)
{
    uint32_t sector;
    int32_t  status = SystemP_SUCCESS;

    status = flash_offsettosector(absolute_offset, &sector);
    if (status != SystemP_SUCCESS)
    {
        return -1;
    }

    if (sector != last_erased_sector)
    {
        // 当前扇区在此次写模式open时，未被擦除
        status = Flash_eraseSector_com(filesystem_flash_handle, sector, flash_chipSelect);
        if (status != SystemP_SUCCESS)
        {
            return -1;
        }

        last_erased_sector = sector;
    }

    return 0;
}

/**
 * @brief            : 记录FAT文件信息
 * @param fileHandle : 文件句柄，也就是FAT表索引
 * @param info       : 要记录的信息
 * @return           : 0-记录失败，1-记录成功
 */
uint8_t file_info_write(int32_t fileHandle, FILE_INFO *info)
{
    uint32_t info_offset_base   = 0;
    uint32_t info_offset_actual = 0;
    uint32_t sectorID;
    int32_t  status = SystemP_SUCCESS;

    if(fileHandle < 0 || fileHandle >= FILE_MAX_NUM || info == NULL)
    {
        return 0;
    }

    switch(File_Fat_Table.File_Fat[fileHandle].Flash_Pro)
    {
    case FLASH_PRO_CPU_IMAGE:
        info_offset_base = APP_IMAGE_INFO_OFFSET_BASE;
        break ;
    case FLASH_PRO_CPU_DATA:
        info_offset_base = CONFIG_FILE_INFO_OFFSET_BASE;
        break ;
    case FLASH_PRO_FPGA_IMAGE:
        info_offset_base = FPGA_IMAGE_INFO_OFFSET_BASE;
        break ;
    default:
        return 0;
    }

    info_offset_actual = info_offset_base + FLASH_SECTOR_SIZE * File_Fat_Table.File_Fat[fileHandle].Flash_Pro_Seq_Num;
    flash_chipSelect = select_flash(fileHandle);

    flash_offsettosector(info_offset_actual, &sectorID);
    status = Flash_eraseSector_com(filesystem_flash_handle, sectorID, flash_chipSelect);
    status = Flash_write_com(filesystem_flash_handle,
                              info_offset_actual, (uint8_t *)info, sizeof(*info), flash_chipSelect);
    if (status != SystemP_SUCCESS)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief            : 获取FAT文件信息
 * @param fileHandle : 文件句柄，也就是FAT表索引
 * @param info       : FAT文件信息
 * @return           : 0-获取失败，1-获取成功
 */
uint8_t file_info_read(int32_t fileHandle, FILE_INFO *info)
{
    uint32_t info_offset_base   = 0;
    uint32_t info_offset_actual = 0;
    int32_t  status = SystemP_SUCCESS;

    if(fileHandle < 0 || fileHandle >= FILE_MAX_NUM || info == NULL)
    {
        return 0;
    }

    switch(File_Fat_Table.File_Fat[fileHandle].Flash_Pro)
    {
    case FLASH_PRO_CPU_IMAGE:
        info_offset_base = APP_IMAGE_INFO_OFFSET_BASE;
        break ;
    case FLASH_PRO_CPU_DATA:
        info_offset_base = CONFIG_FILE_INFO_OFFSET_BASE;
        break ;
    case FLASH_PRO_FPGA_IMAGE:
        info_offset_base = FPGA_IMAGE_INFO_OFFSET_BASE;
        break ;
    default:
        return 0;
    }

    info_offset_actual = info_offset_base + FLASH_SECTOR_SIZE * File_Fat_Table.File_Fat[fileHandle].Flash_Pro_Seq_Num;
    flash_chipSelect = select_flash(fileHandle);

    status = Flash_read_com(filesystem_flash_handle,
                             info_offset_actual, (uint8_t *)info, sizeof(*info), flash_chipSelect);
    if (status != SystemP_SUCCESS)
    {
        return 0;
    }

    if((info->Use_Flag == 0xA5) && ((check_sum_32bit((uint8_t *)info, sizeof(FILE_INFO) - 4)) == info->Check_Sum))
    {
        return 1;
    }
    return 0;
}

/**
 * @brief            : 在FAT表中查找文件
 *                     FAT文件名后面定义的相应字符匹配即可 (如下载文件名为2025-05-22_V2_Core1.bin，FAT表字符为Core1.bin也算匹配)
 * @param p_filename : 要打开的文件名
 * @param mode       : 打开的模式 (FAT_MODE_R_OPEN-只读 FAT_MODE_W_OPEN-只写)
 * @return           : 文件句柄，也就是FAT表索引 (-1：FAT表校验错误，-2：在FAT表中未找到目标文件，-3：有文件未关闭或参数错误，-4：Flash操作失败)
 * @attention        : 当操作文件时，必须一个一个的文件操作，不能一次打开多个文件进行操作 (因为PageBuffer是所有文件公用的)
 */
int32_t file_open(const char *p_filename, uint8_t mode)
{
    FILE_FAT_TABLE_STRUCT *pfat_tab  = &File_Fat_Table;
    EACH_FLASHFAT_STRUCT  *pfat_item = NULL;
    uint32_t fat_sum                 = 0;
    uint32_t sectorID_base           = 0, offset_base = 0;
    int32_t  index                   = 0;
    int32_t  status                  = SystemP_SUCCESS;

    if((p_filename == NULL) || (!(mode == FAT_MODE_R_OPEN || mode == FAT_MODE_W_OPEN)))
    {
        return -3;
    }

    // 比较FAT表的校验和
//    fat_sum = check_sum_32bit((uint8_t *)pfat_tab, sizeof(*pfat_tab) - 4);
//    if(fat_sum != pfat_tab->Fat_sum)
//    {
//        return -1;
//    }

    // 在FAT表的有效表项中查找目标文件
    for( ; index < pfat_tab->FileNum; index++)
    {
        pfat_item = &pfat_tab->File_Fat[index];
        if(compare_filename(p_filename, pfat_item->Name, pfat_item->Flash_Pro))
        {
            // 找到目标文件
            if(fileHandle_last != 0xFF)
            {
                file_close(fileHandle_last, 0); //close时，file_src_sum填什么数据都没关系，因为后面会重新close
            }

            if((FatFile_Flag[index] != FAT_ITEM_CLOSE) || (PageBuffer_size != 0) || (file_checksum != 0)
                                                       || (file_size != 0) || (last_erased_sector != 0xFFFF))
            {
                return -3;
            }
            FatFile_Flag[index] = FAT_ITEM_R_OPEN;
            break;
        }
    }

    if(index >= pfat_tab->FileNum)
    {
        return -2;
    }

    if(FAT_MODE_W_OPEN == mode)
    {
        FatFile_Flag[index] = FAT_ITEM_W_OPEN;
    }

    fileHandle_last = index;
    return index;
}

/**
 * @brief               : 向flash中写入数据 (可按整个文件一次性写入，也可以将文件分为几次写)
 *                        如果不满一页则先将数据暂存 (不满一页的数据暂存的原因：flash只支持页写，传入写flash函数的绝对写偏移地址必须是页对齐)
 *                        每次写flash之前要判断此扇区是否被擦除，如果没有则擦除
 * @param  ptr          : 缓存区内容指针
 * @param  offset       : 要写的数据在本文件中的偏移，0开始 (byte为单位)
 * @param  size_of_file : 要写入的数据大小 (byte为单位)
 * @param  fileHandle   : 文件句柄，也就是FAT表索引
 * @return              : 写入flash数据长度，-1：写入失败 (暂存进PageBuffer的也算)或擦除失败，-2：要写的文件不是当前打开的文件
 * @attention           : 如果是一个文件分多次写入的情况，传入的数据必须连续 (例如文件第一次从0偏移写入100字节，第二次必须从100偏移开始)
 */
int32_t file_write(uint8_t *ptr, uint32_t offset, uint32_t size_of_file, int32_t fileHandle)
{
    FILE_FAT_TABLE_STRUCT *pfat_tab  = &File_Fat_Table;
    EACH_FLASHFAT_STRUCT  *pfat_item = &pfat_tab->File_Fat[fileHandle];
    uint32_t sectorID_base = 0, offset_base = 0, absolute_offset = 0;
    uint32_t size_all = 0, num = 0, remainder = 0;
    int32_t ret = 0;
    int32_t status = SystemP_SUCCESS;

    if(fileHandle < 0 ||
        fileHandle >= FILE_MAX_NUM ||
        (FAT_ITEM_W_OPEN != FatFile_Flag[fileHandle]) ||
        (ptr == NULL) || (size_of_file <= 0))
    {
        return -1;
    }

    if(fileHandle_last != fileHandle)
    {
        //要写的文件不是当前打开的文件
        return -2;
    }

    get_absolute_sectorID_offset(fileHandle, &sectorID_base, &offset_base);
    absolute_offset = offset_base + pfat_item->Sector_ID * FLASH_SECTOR_SIZE + offset - PageBuffer_size;
    size_all = size_of_file + PageBuffer_size;
    num = size_all / FLASH_PAGE_SIZE;
    remainder = size_all % FLASH_PAGE_SIZE;
    for(uint32_t i = 0; i < num; i++)
    {
        memcpy(&PageBuffer[PageBuffer_size], ptr, FLASH_PAGE_SIZE - PageBuffer_size);
        file_checksum += check_sum_32bit(PageBuffer, FLASH_PAGE_SIZE);
        flash_chipSelect = select_flash(fileHandle);

        //写flash之前擦除要写的扇区
        status = flash_erase_sector(absolute_offset);
        if (status != SystemP_SUCCESS)
        {
            return -1;
        }

        status = Flash_write_com(filesystem_flash_handle,
                                 absolute_offset,
                                 PageBuffer,
                                 FLASH_PAGE_SIZE, flash_chipSelect);
        if (status != SystemP_SUCCESS)
        {
            Debug_logError("write file at 0x%08x\n", file_offset);
            return -1;
        }

#if DEBUG_FILE_SYSTEM
        status = Flash_read_com(filesystem_flash_handle,
                                absolute_offset,
                                PageBufferBak,
                                FLASH_PAGE_SIZE);
        if (status != SystemP_SUCCESS)
        {
            Debug_logError("write file at 0x%08x\n", file_offset);
            return -1;
        }

        if (memcmp(PageBuffer, PageBufferBak, FLASH_PAGE_SIZE) != 0)
        {
            Debug_logError("write file at 0x%08x\n", absolute_offset);
            return -3;
        }
#endif

        size_all -= FLASH_PAGE_SIZE;
        absolute_offset += FLASH_PAGE_SIZE;
        ptr = ptr + (FLASH_PAGE_SIZE - PageBuffer_size);
        PageBuffer_size = 0;
        file_size += FLASH_PAGE_SIZE;
    }

    if((remainder > 0) && (remainder == size_all))// 顺便验证一下size_all的值
    {
        if(num > 0)
        {
            memcpy(&PageBuffer[PageBuffer_size], ptr, remainder);
        }
        else
        {
            memcpy(&PageBuffer[PageBuffer_size], ptr, size_of_file);
        }
        PageBuffer_size = remainder;
    }

    file_offset = absolute_offset;// 记录下一次PageBuffer数据要写入的偏移
    ret = (int32_t)size_of_file;
    return ret;
}

/**
 * @brief               : 读文件
 * @param  ptr          : 缓存区内容指针
 * @param  offset       : 要读的数据在本文件中的偏移，0开始 (byte为单位)
 * @param  size_of_file : 要读出的数据大小 (byte为单位)
 * @param  fileHandle   : 文件句柄，也就是FAT表索引
 * @return              : 实际读出的长度，-1：读出错，-2：要读的文件不是当前打开的文件
 */
int32_t file_read(uint8_t *ptr, uint32_t offset, uint32_t size_of_file, int32_t fileHandle)
{
    FILE_FAT_TABLE_STRUCT *pfat_tab  = &File_Fat_Table;
    EACH_FLASHFAT_STRUCT  *pfat_item = &pfat_tab->File_Fat[fileHandle];
    uint32_t sectorID_base = 0, offset_base = 0, absolute_offset = 0;
    int32_t ret = 0;
    int32_t status = SystemP_SUCCESS;

    if(fileHandle < 0 || fileHandle >= FILE_MAX_NUM || (FAT_ITEM_R_OPEN != FatFile_Flag[fileHandle])
                                                    || (ptr == NULL) || (size_of_file <= 0))
    {
        return -1;
    }

    if(fileHandle_last != fileHandle)
    {
        //要读的文件不是当前打开的文件
        return -2;
    }

    get_absolute_sectorID_offset(fileHandle, &sectorID_base, &offset_base);
    absolute_offset = offset_base + pfat_item->Sector_ID * FLASH_SECTOR_SIZE + offset;
    flash_chipSelect = select_flash(fileHandle);

    status = Flash_read_com(filesystem_flash_handle,
                             absolute_offset, ptr, size_of_file, flash_chipSelect);
    if (status != SystemP_SUCCESS)
    {
        return -1;
    }

    ret = (int32_t)size_of_file;

    return ret;
}

/**
 * @brief              : 关闭file_open打开的文件，若为写操作打开则记录文件信息到flash
 *                       如果PageBuffer中有暂存的数据，则在close时，将暂存的不满一页的数据写入flash
 * @param fileHandle   : 文件句柄，也就是FAT表索引
 * @param file_src_sum : 上位机下发的整个文件32bit数据校验和，如果是以读模式打开后关闭文件，则填0即可
 * @return             : fileHandle文件实际写入flash的数据个数，-1：出错 (如果是以读模式打开文件，则close时返回0)，
 *                       -2：要关闭的文件不是当前打开的文件，-3：下位机计算的文件32bit数据校验和与上位机下发的数据校验和不一致
 * @attention          : 如果下位机计算的文件32bit数据校验和与上位机下发的数据校验和不一致时，也要把file info信息存入FLASH
 */
int32_t file_close(int32_t fileHandle, uint32_t file_src_sum)
{
    FILE_FAT_TABLE_STRUCT *pfat_tab  = &File_Fat_Table;
    EACH_FLASHFAT_STRUCT  *pfat_item = &pfat_tab->File_Fat[fileHandle];
    int32_t ret = 0;
    int32_t status = SystemP_SUCCESS;

    if(fileHandle < 0 || fileHandle >= FILE_MAX_NUM || (FatFile_Flag[fileHandle] == FAT_ITEM_CLOSE))
    {
        return -1;
    }

    if(fileHandle_last != fileHandle)
    {
        //要关闭的文件不是当前打开的文件
        return -2;
    }

    if(FAT_ITEM_R_OPEN == FatFile_Flag[fileHandle])
    {
        FatFile_Flag[fileHandle] = FAT_ITEM_CLOSE;
        fileHandle_last = 0xFF;
        return 0;
    }

    // 将PageBuffer中暂存的数据写入flash
    if(PageBuffer_size > 0)
    {
        file_checksum += check_sum_32bit(PageBuffer, PageBuffer_size);
        flash_chipSelect = select_flash(fileHandle);

        //写flash之前擦除要写的扇区
        status = flash_erase_sector(file_offset);
        if (status != SystemP_SUCCESS)
        {
            return -1;
        }

#if DEBUG_FILE_SYSTEM
        status = Flash_write_com(filesystem_flash_handle,
                                 file_offset, PageBuffer,
                                 PageBuffer_size);
        if (status != SystemP_SUCCESS)
        {
            Debug_logError("write file at 0x%08x\n", file_offset);
            return -3;
        }

        status = Flash_read_com(filesystem_flash_handle,
                                file_offset,
                                PageBufferBak,
                                PageBuffer_size);
        if (status != SystemP_SUCCESS)
        {
            Debug_logError("write file at 0x%08x\n", file_offset);
            return -3;
        }

        if (memcmp(PageBuffer, PageBufferBak, PageBuffer_size) != 0)
        {
            Debug_logError("write file at 0x%08x\n", file_offset);
            return -3;
        }
#endif

        status = Flash_write_com(filesystem_flash_handle,
                                  file_offset, PageBuffer, PageBuffer_size, flash_chipSelect);
        if (status != SystemP_SUCCESS)
        {
            return -1;
        }

        file_size += PageBuffer_size;
    }

    memset(&File_Info, 0, sizeof(File_Info));
    strcpy(File_Info.Name, pfat_item->Name);
//    File_Info.Time //时间先写死全为0，因为时间应该由Core0统一对时管理，由它维护RTC和B码对时功能，它把时间信息放到共享内存中，其它Core共享
    File_Info.Size         = file_size;
    File_Info.File_Src_Sum = file_src_sum;
    File_Info.File_Sum     = file_checksum;
    File_Info.Use_Flag     = 0xA5;
    File_Info.Check_Sum    = check_sum_32bit((uint8_t *)&File_Info, sizeof(FILE_INFO) - 4);
    file_info_write(fileHandle, &File_Info);

    ret = (int32_t)file_size;
    FatFile_Flag[fileHandle] = FAT_ITEM_CLOSE;
    PageBuffer_size = 0;
    file_checksum = 0;
    file_size = 0;

    fpga_flash_switch(0);

    fileHandle_last    = 0xFF;
    last_erased_sector = 0xFFFF;

    // 当下位机计算的文件32bit数据校验和与上位机下发的数据校验和不一致时，返回错误
    if(File_Info.File_Src_Sum != File_Info.File_Sum)
    {
        return -3;
    }
    return ret;
}