/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       backtrace.h
*@author     LiuRui
*@date       2025.03.22
*@brief
*@par        History
*Date        Version   Author     Description
*2025.03.22  1.0       LiuRui
******************************************************************************/

#ifndef BACKTRACE_H
#define BACKTRACE_H


#include <stdint.h>
#include <stdbool.h>

#define FREERTOS 1



uint32_t backtrace_call_stack(uint32_t *buffer, uint32_t buf_depth, uint32_t sp);
void dump_backtrace();
void dabt_exptn_handler(void *ptr);

#endif  // BACKTRACE_H
