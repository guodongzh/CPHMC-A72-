/**
 *************************************************************************
 * @file      dma.h
 * @author    LiuRui
 * @date      2024/5/24
 * @version   V1.0
 * @board     ti_j721e_evm
 * @brief     memory to memory dma test
 *************************************************************************
 */


#ifndef DMA_H
#define DMA_H

#include <stdio.h>
#include <ti/drv/uart/UART.h>
#include <ti/drv/uart/UART_stdio.h>
#include "udma_apputils.h"
#include <ti/drv/udma/udma.h>
#include "debug_config.h"


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*
 * Application test parameters
 */
/** \brief Number of bytes to copy and buffer allocation */
#define UDMA_TEST_APP_NUM_BYTES (1024U * 1)
/** \brief This ensures every channel memory is aligned     Cache line aligned 128byte   */
#define UDMA_TEST_APP_NUM_BYTES_ALIGN                                          \
  ((UDMA_TEST_APP_NUM_BYTES + UDMA_CACHELINE_ALIGNMENT) &                      \
   ~(UDMA_CACHELINE_ALIGNMENT - 1U))

/** \brief Number of times to perform the memcpy operation*/
#define UDMA_TEST_APP_LOOP_CNT (20U)

/*
 * Ring parameters
 */
/** \brief Number of ring entries - we can prime this much memcpy operations */
#define UDMA_TEST_APP_RING_ENTRIES UDMA_TEST_APP_LOOP_CNT
/** \brief Size (in bytes) of each ring entry (Size of pointer - 64-bit) */
#define UDMA_TEST_APP_RING_ENTRY_SIZE (sizeof(uint64_t))
/** \brief Total ring memory */
#define UDMA_TEST_APP_RING_MEM_SIZE                                            \
  (UDMA_TEST_APP_RING_ENTRIES * UDMA_TEST_APP_RING_ENTRY_SIZE)
/** \brief This ensures every channel memory is aligned */
#define UDMA_TEST_APP_RING_MEM_SIZE_ALIGN                                      \
  ((UDMA_TEST_APP_RING_MEM_SIZE + UDMA_CACHELINE_ALIGNMENT) &                  \
   ~(UDMA_CACHELINE_ALIGNMENT - 1U))
/**
 *  \brief UDMA TR packet descriptor memory.
 *  This contains the CSL_UdmapCppi5TRPD + Padding to sizeof(CSL_UdmapTR15) +
 *  one Type_15 TR (CSL_UdmapTR15) + one TR response of 4 bytes.
 *  Since CSL_UdmapCppi5TRPD is less than CSL_UdmapTR15, size is just two times
 *  CSL_UdmapTR15 for alignment.
 */
#define UDMA_TRPD_SIZE ((sizeof(CSL_UdmapTR15) * 2U) + 4U)
/** \brief This ensures every channel memory is aligned */
#define UDMA_TRPD_SIZE_ALIGN                                          \
  ((UDMA_TRPD_SIZE + UDMA_CACHELINE_ALIGNMENT) &                      \
   ~(UDMA_CACHELINE_ALIGNMENT - 1U))

/* Enable interrupt instead of polling mode */
#define UDMA_TEST_INTR 0

int32_t udma_setup(void);
int32_t udma_memcpy(void *destBuf, void *srcBuf, uint32_t length);
int32_t udma_memcpy_wait_complete(uint32_t time_out);
int32_t udma_memcpy_test(void);


#endif // DMA_H
