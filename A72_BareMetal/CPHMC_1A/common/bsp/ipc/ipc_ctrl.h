/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       ipc_ctrl.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 to C7X IPC control interface.
 *@par        History
 *Date        Version   Author       Description
 *2026.09.11  1.0       zhaoguodong  Create file.
 ******************************************************************************/

#ifndef IPC_CTRL_H
#define IPC_CTRL_H

#include <stdint.h>

int32_t ipc_ctrl_init(void);
int32_t ipc_ctrl_ping(void);
int32_t ipc_ctrl_poll(void);
uint32_t ipc_ctrl_get_rx_count(void);
uint32_t ipc_ctrl_get_message_count(void);

#endif /* IPC_CTRL_H */
