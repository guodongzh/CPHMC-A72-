/******************************************************************************
*@copyright  Copyright(c) 2025 RXHK.Co.,Ltd.
*@file       net_muticast.c
*@author     xuesen
*@date       2026.05.06
*@brief      组播IP地址检查实现。
*@par        History
*Date        Version   Author     Description
*2026.05.06  1.0       xuesen     Initial version
******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/

#include "net_all_include.h"

#ifdef __cplusplus
extern "C" {

#endif

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */

/**
 * @brief 检查IP地址是否在组播表中。
 * @param ipaddr 待检查IP地址
 * @return true表示在表中，false表示不在表中
 */


bool muti_chklist(const ipaddr_t *ipaddr)
{
    uint16_t i;

    //	aeos_assert(!(NULL == ipaddr));

    for (i = 0; i < ARRAYSIZEOF(MultiIPTbl); i++)
    {
        if (ipaddr_cmp(all_zeroes_ipaddr, MultiIPTbl[i]))
        {
            break;
        }
        if (ipaddr_cmp(MultiIPTbl[i], (*ipaddr)))
        {
            return true;
        }
    }
    return false;
}
#ifdef __cplusplus
}
#endif