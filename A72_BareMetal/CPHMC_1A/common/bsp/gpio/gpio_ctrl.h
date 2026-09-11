/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       gpio_ctrl.h
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      GPIO control interface for the A72 No-OS image.
 *@par        History
 *Date        Version   Author         Description
 *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************/

#ifndef GPIO_CTRL_H
#define GPIO_CTRL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#include "bsp/gpio/gpio_intr_init.h"

#include <ti/csl/soc/j721e/src/cslr_soc.h>
#include <ti/csl/soc/j721e/src/cslr_intr_compute_cluster0_gic500ss.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/drv/gpio/src/v0/GPIO_v0.h>
#include <ti/drv/gpio/soc/GPIO_soc.h>

#define GPIO_CTRL_PORT              (0U)
#define GPIO_CTRL_BOOTMODE0_PIN     (6U)
#define GPIO_CTRL_BOOTMODE2_PIN     (48U)
#define GPIO_CTRL_SHARED_INTR_IRQ   \
                                    (CSLR_COMPUTE_CLUSTER0_GIC500SS_SPI_GPIOMUX_INTRTR0_OUTP_41)


#define GPIO_CTRL_SYS_BOOTMODE0     GPIO_CTRL_BOOTMODE0_PIN /* AD20, GPIO0_6  */
#define GPIO_CTRL_SYS_BOOTMODE2     GPIO_CTRL_BOOTMODE2_PIN /* AC29, GPIO0_48 */

/* DSPC7X and A72 share GPIO0_29 through GPIOMUX_INTRTR0_OUTP_41. */
#define GPIO_CTRL_SHARED_INTR_PIN    (29U)
#define GPIO_CTRL_SHARED_INTR_INDEX  (0U)

/* Initializes GPIO0_29 as an interrupt input and both BOOTMODE GPIOs low. */
int32_t gpio_ctrl_init(void);


#ifdef __cplusplus
}
#endif

#endif /* GPIO_CTRL_H */
