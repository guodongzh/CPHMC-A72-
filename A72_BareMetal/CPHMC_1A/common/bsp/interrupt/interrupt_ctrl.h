/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       interrupt_ctrl.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 generic-timer hard-interrupt control.
 *@par        History
 *Date        Version   Author     Description
 *2026.09.11  1.0       zhaoguodong Initial version
 ******************************************************************************/
#ifndef _INTERRUPT_CTRL_H
#define _INTERRUPT_CTRL_H

#include <stdint.h>

#define INTERRUPT_CTRL_HARD_IRQ  (30U)
#define INTERRUPT_CTRL_SOFT_IRQ  (1U)

int32_t interrupt_ctrl_init(void);
void interrupt_ctrl_reload_timer(void);
void interrupt_ctrl_enable(void);

#endif
