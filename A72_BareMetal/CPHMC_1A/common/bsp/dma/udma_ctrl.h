/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       udma_ctrl.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      UDMA block-copy control interface for the A72 No-OS image.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#ifndef UDMA_CTRL_H
#define UDMA_CTRL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the TI UDMA driver and one MAIN NAVSS block-copy channel. */
int32_t udma_ctrl_init(void);

/*
 * Submit one synchronous memory-to-memory transfer.
 * timeoutLoops is the maximum number of completion-ring polls.
 */
int32_t udma_ctrl_memcpy(void *destination,
                         const void *source,
                         uint32_t length,
                         uint32_t timeoutLoops);

/* Run the official-style 1 KiB DDR-to-DDR UDMA copy and compare test. */
int32_t udma_ctrl_self_test(void);

#ifdef __cplusplus
}
#endif

#endif /* UDMA_CTRL_H */
