/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_intr_init.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      Shared GPIO external-interrupt interface for A72 No-OS.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#ifndef GPIO_INTR_INIT_H
#define GPIO_INTR_INIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t gpio_intr_init(void);
uint32_t gpio_intr_get_count(void);

/* TI GPIO LLD callback; application code should not call it directly. */
void gpio_intr_callback(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_INTR_INIT_H */
