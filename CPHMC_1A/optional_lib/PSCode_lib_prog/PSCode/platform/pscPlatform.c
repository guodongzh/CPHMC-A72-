/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pscPlatform.c
 *@author     LiuRui
 *@date       2026.02.03
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2026.02.03  1.0       LiuRui
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "pscPlatform.h"
#include "pscCom.h"
#include "ipc_data_drv.h"
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
#ifdef BUILD_C66X
tPscSetData setSingleDataLocal;
tPscWatchTable pWatchTablesLocal __attribute__((aligned(128))); //Watch table manager
PSCDWORD       watchDataLocal[PSCMAXWATCH] __attribute__((aligned(128))); //Watch data
tPscSetData *setDataTablePtr = &setSingleDataLocal;
PSCDWORD *pWatchData = watchDataLocal;
tPscWatchTable* pWatchTable = (&(pWatchTablesLocal));

psc_wave           g_psc_wave_list_c6x __attribute__((aligned(128))) = {0}; // MSMC -> C6x
psc_wave_data_tmp  g_psc_wave_data_c6x[PSC_WAVE_MAX_CHANNEL_NUM] __attribute__((aligned(128))) = {0}; // C6x -> MSMC
#else
tPscSetData *setDataTablePtr = &SetDataTable;
tPscWatchTable* pWatchTable = &pWatchTables_g;
PSCDWORD *pWatchData = watchData_g;
#endif
psc_wave_data_tmp_info g_psc_wave_temp_info __attribute__((section(".wave_info_tmp")));
psc_wave_data_tmp      g_psc_wave_data_tmp[PSC_WAVE_DATA_TMP_NUM][PSC_WAVE_MAX_CHANNEL_NUM] __attribute__((section(".wave_data_tmp")));
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
#if defined(SOC_J721E)

PSCBYTE g_core_name[PSC_CORE_NUM][VERSION_STRING_SIZE / 2] =
    {"R5F0", "R5F1", "R5F2", "R5F3", "DSPC6x0", "DSPC6x1", "DSPC7x0", "MCU1"};

char g_hardware_name[SIZE_PLATFORM_NAME] = {PLATFORM_NAME "_" CORE_NAME};
char g_platform_name[SIZE_PLATFORM_NAME] = {PLATFORM_NAME "_" CORE_NAME};

uint8_t PscGetCoreId(void)
{
    return CORE_NR;
}

void PscResVersionShare(void)
{
    uint8_t flag = 0xAC;
    ipc_data_drv_write(1, (uint8_t *)&resVersion_g, sizeof(resVersion_g));
    ipc_data_drv_write(0, (uint8_t *)&flag, sizeof (flag));
}

void PscGetResVersionShare(void)
{
    static uint8_t get_ver_flag = 0x7F;
    uint8_t flag                = 0;

    if (get_ver_flag)
    {
        if (get_ver_flag & 0x01)
        {
            ipc_data_drv_read(r1, 0, (uint8_t *)&flag, sizeof (flag));
            if (flag == 0xAC)
            {
                ipc_data_drv_read(r1, 1, (uint8_t *)&resVersionMultiCore[1], sizeof (resVersionMultiCore[1]));
                get_ver_flag &= ~0x01;
                flag = 0;
            }
        }

        if (get_ver_flag & 0x02)
        {
            ipc_data_drv_read(r2, 0, (uint8_t *)&flag, sizeof (flag));
            if (flag == 0xAC)
            {
                ipc_data_drv_read(r2, 1, (uint8_t *)&resVersionMultiCore[2], sizeof (resVersionMultiCore[2]));
                get_ver_flag &= ~0x02;
                flag = 0;
            }
        }

        if (get_ver_flag & 0x04)
        {
            ipc_data_drv_read(r3, 0, (uint8_t *)&flag, sizeof (flag));
            if (flag == 0xAC)
            {
                ipc_data_drv_read(r3, 1, (uint8_t *)&resVersionMultiCore[3], sizeof (resVersionMultiCore[3]));
                get_ver_flag &= ~0x04;
                flag = 0;
            }
        }

        if (get_ver_flag & 0x08)
        {
            ipc_data_drv_read(c60, 0, (uint8_t *)&flag, sizeof (flag));
            if (flag == 0xAC)
            {
                ipc_data_drv_read(c60, 1, (uint8_t *)&resVersionMultiCore[4], sizeof (resVersionMultiCore[4]));
                get_ver_flag &= ~0x08;
                flag = 0;
            }
        }

        if (get_ver_flag & 0x10)
        {
            ipc_data_drv_read(c61, 0, (uint8_t *)&flag, sizeof (flag));
            if (flag == 0xAC)
            {
                ipc_data_drv_read(c61, 1, (uint8_t *)&resVersionMultiCore[5], sizeof (resVersionMultiCore[5]));
                get_ver_flag &= ~0x10;
                flag = 0;
            }
        }

        if (get_ver_flag & 0x20)
        {
            ipc_data_drv_read(c70, 0, (uint8_t *)&flag, sizeof (flag));
            if (flag == 0xAC)
            {
                ipc_data_drv_read(c70, 1, (uint8_t *)&resVersionMultiCore[6], sizeof (resVersionMultiCore[6]));
                get_ver_flag &= ~0x20;
                flag = 0;
            }
        }

        if (get_ver_flag & 0x40)
        {
            ipc_data_drv_read(mcu1, 0, (uint8_t *)&flag, sizeof (flag));
            if (flag == 0xAC)
            {
                ipc_data_drv_read(mcu1, 1, (uint8_t *)&resVersionMultiCore[7], sizeof (resVersionMultiCore[7]));
                get_ver_flag &= ~0x40;
                flag = 0;
            }
        }
    }
}

uint32_t PscGetCoreIsrTime(void)
{
    return ((uint32_t *)&init_param)[g_pscode_udp_info.pscode_src_port - PORT_ID_BASE];
}

void PscCacheInv(const void *addr, uint32_t size)
{
    CacheP_Inv(addr, size);
}

void PscCacheWb(const void *addr, uint32_t size)
{
    CacheP_wb(addr, size);
}

void PscSetVariableStart()
{
#if defined(BUILD_C66X)
    udma_memcpy((uint8_t *)&setSingleDataLocal, (uint8_t *)&SetDataTable, sizeof(tPscSetData));
    udma_memcpy_wait_complete(0xffffffff);
#endif
}

void PscSetVariableEnd()
{
#if defined(BUILD_C66X)
    udma_memcpy((uint8_t *)&SetDataTable, (uint8_t *)setDataTablePtr, sizeof(tPscSetData));
#endif
}

void PscGetWatchDataStart()
{
#if defined(BUILD_C66X)
    udma_memcpy((uint8_t *)&pWatchTablesLocal, (uint8_t *)&pWatchTables_g, sizeof(tPscWatchTable));
//    udma_memcpy_wait_complete(0xffffffff);
#endif
}

void PscGetWatchDataEnd()
{
#if defined(BUILD_C66X)
    udma_memcpy((uint8_t *)&watchData_g, (uint8_t *)pWatchData, sizeof(watchData_g));
#endif
}

void PscGetWaveStart()
{
#if defined(BUILD_C66X)
    udma_memcpy((uint8_t *)&g_psc_wave_list_c6x, (uint8_t *)&g_psc_wave_list, sizeof(g_psc_wave_list_c6x));
#endif
}

void PscGetWaveEnd(uint32_t tmp_index, psc_wave_data_tmp_info *info)
{
#if defined(BUILD_C66X)
    udma_memcpy((uint8_t *)&g_psc_wave_data_tmp[tmp_index],
                (uint8_t *)&g_psc_wave_data_c6x,
                sizeof(g_psc_wave_data_c6x));
    udma_memcpy((uint8_t *)&g_psc_wave_temp_info,
                (uint8_t *)info,
                sizeof(g_psc_wave_temp_info));
#endif
}


#elif defined(SOC_AM64X)
char g_core_name[PSC_CORE_NUM][VERSION_STRING_SIZE / 2] =
    {"R5F0", "R5F1", "R5F2", "R5F3"};

char g_hardware_name[SIZE_PLATFORM_NAME] = {PLATFORM_NAME "_" CORE_NAME};
char g_platform_name[SIZE_PLATFORM_NAME] = {PLATFORM_NAME "_" CORE_NAME};

uint8_t PscGetCoreId(void)
{
    return CORE_NR;
}


void PscCacheInv(const void *addr, uint32_t size)
{
    CacheP_inv(addr, size, CacheP_TYPE_ALL);
}

void PscCacheWb(const void *addr, uint32_t size)
{
    CacheP_wb(addr, size, CacheP_TYPE_ALL);
}

#endif





