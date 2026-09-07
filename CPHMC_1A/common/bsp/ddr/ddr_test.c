/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       ddr_test.c
 *@author     xuesen
 *@date       2024.08.28
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2024.08.28  1.0       wenjunf    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "ddr_test.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
void ddr_test(void)
{
    uint32_t           pattern[] = {0x00000000, 0xFFFFFFFF, 0xAAAAAAAA, 0x55555555};
    volatile uint32_t *ddr_addr;
    uint32_t           read_data;

    for (int i = 0; i < sizeof(pattern) / sizeof(pattern[0]); i++)
    {
        ddr_addr = (uint32_t *)DDR_BASE_ADDR;
        for (uint32_t j = 0; j < DDR_SIZE / sizeof(uint32_t); j++)
        {
            *ddr_addr++ = pattern[i];
        }

        // 读回并验证
        ddr_addr = (uint32_t *)DDR_BASE_ADDR;
        for (uint32_t j = 0; j < DDR_SIZE / sizeof(uint32_t); j++)
        {
            read_data = *ddr_addr++;
            if (read_data != pattern[i])
            {
                Debug_log("test failed addr: %p, write: 0x%08X, read: 0x%08X\n",
                          ddr_addr,
                          pattern[i],
                          read_data);
                return;
            }
        }
    }
    Debug_log("DDR 1KB test ok!!\n");
}
