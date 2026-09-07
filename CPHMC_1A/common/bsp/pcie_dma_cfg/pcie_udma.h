/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_udma.h
 *@author     LiuRui
 *@date       2025.09.25
 *@brief
 *@par        History
 *Date        Version   Author     Description
 *2025.09.25  1.0       LiuRui
 ******************************************************************************/

#ifndef _CPHMC_1A_C7X_PCIE_UDMA_H
#define _CPHMC_1A_C7X_PCIE_UDMA_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <ti/drv/udma/udma.h>
#include "udma_apputils.h"

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
/**
 * Ring parameters
 */
/**  Number of ring entries - we can prime this much memcpy operations */
#define UDMA_RING_ENTRIES      1

/**  Size (in bytes) of each ring entry (Size of pointer - 64-bit) */
/** Total ring memory */
#define UDMA_RING_MEM_SIZE     (UDMA_RING_ENTRIES * (sizeof(uint64_t)))

/** This ensures every channel memory is aligned */
#define UDMA_RING_MEM_SIZE_ALIGN                       \
    ((UDMA_RING_MEM_SIZE + UDMA_CACHELINE_ALIGNMENT) & \
     ~(UDMA_CACHELINE_ALIGNMENT - 1U))
/**
 *  UDMA TR packet descriptor memory.
 *  This contains the CSL_UdmapCppi5TRPD + Padding to sizeof(CSL_UdmapTR15) +
 *  one Type_15 TR (CSL_UdmapTR15) + one TR response of 4 bytes.
 *  Since CSL_UdmapCppi5TRPD is less than CSL_UdmapTR15, size is just two times
 *  CSL_UdmapTR15 for alignment.
 */
#define UDMA_TRPD_SIZE ((sizeof(CSL_UdmapTR15) * 2U) + 4U)

/** This ensures every channel memory is aligned */
#define UDMA_TRPD_SIZE_ALIGN                       \
    ((UDMA_TRPD_SIZE + UDMA_CACHELINE_ALIGNMENT) & \
     ~(UDMA_CACHELINE_ALIGNMENT - 1U))



/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif  //_CPHMC_1A_C7X_PCIE_UDMA_H
