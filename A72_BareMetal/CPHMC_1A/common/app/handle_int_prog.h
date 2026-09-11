/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       handle_int_prog.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      Hard-interrupt and software-interrupt application interface.
 *@par        History
 *Date        Version   Author     Description
 *2026.09.11  1.0       zhaoguodong Initial version
 ******************************************************************************/
#ifndef _HANDLE_INT_PROG_H
#define _HANDLE_INT_PROG_H

#include <stdint.h>

#define APP_SOFT_INT_TIME_US (1000U)

void handle_hard_int_prog(uintptr_t arg);
void handle_soft_int_prog(uintptr_t arg);

#endif
