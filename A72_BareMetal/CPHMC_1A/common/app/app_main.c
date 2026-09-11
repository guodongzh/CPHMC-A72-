/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       app_main.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      A72 application layer built on hard and software interrupts.
 *@par        History
 *Date        Version   Author     Description
 *2026.09.11  1.0       zhaoguodong Initial version
 ******************************************************************************/

#include "app_main.h"

#include <stdint.h>
#include <stdio.h>

#include <ti/drv/uart/UART_stdio.h>

#include "app_register.h"
#include "handle_int_prog.h"
#include "bsp/dma/udma_ctrl.h"
#include "bsp/gpio/gpio_ctrl.h"
#include "bsp/gpio/gpio_intr_init.h"
#include "bsp/interrupt/interrupt_ctrl.h"
#include "bsp/ipc/ipc_ctrl.h"
#include "bsp/uart/uart_ctrl.h"

static uint32_t g_toggleCount;
static uint32_t g_previousGpioIntrCount;
static int32_t g_ipcStatus;

static void app_loop_task(uintptr_t arg)
{
    (void)arg;
    if (g_ipcStatus == 0)
    {
        (void)ipc_ctrl_poll();
    }
}

static void app_periodic_intr_task(uintptr_t arg)
{
    (void)arg;
    /* Keep this callback short; it is executed from a software-posted HWI. */
}

static void app_1s_task(uintptr_t arg)
{
    uint32_t bootmode0Value;
    uint32_t bootmode2Value;
    uint32_t gpioIntrCount;
    uint32_t ipcMessageCount;
    uint32_t ipcReplyCount;

    GPIO_toggle(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE0_PIN);
    GPIO_toggle(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE2_PIN);
    bootmode0Value = GPIO_read(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE0_PIN);
    bootmode2Value = GPIO_read(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE2_PIN);
    gpioIntrCount = gpio_intr_get_count();
    ipcMessageCount = ipc_ctrl_get_message_count();
    ipcReplyCount = ipc_ctrl_get_rx_count();

    if (g_ipcStatus == 0 && ipcReplyCount == 0U)
    {
        (void)ipc_ctrl_ping();
    }

    g_toggleCount++;
    printf("toggle=%u, SYS_BOOTMODE0=%u, SYS_BOOTMODE2=%u, "
           "GPIO0_29 irq=%u (+%u), IPC rx=%u, pong=%u\n",
           g_toggleCount,
           bootmode0Value,
           bootmode2Value,
           gpioIntrCount,
           gpioIntrCount - g_previousGpioIntrCount,
           ipcMessageCount,
           ipcReplyCount);
    g_previousGpioIntrCount = gpioIntrCount;
}

static int32_t app_register_tasks(void)
{
    if (app_register_loop_task(app_loop_task, 1000U, 100000U) == 0U)
    {
        return -1;
    }
    if (app_register_loop_task(app_1s_task, 1000000U, 5000000U) == 0U)
    {
        return -1;
    }
    if (app_register_intr_task(app_periodic_intr_task, 5U, 1000U) == 0U)
    {
        return -1;
    }
    return app_register_intr();
}

int32_t app_main_init(void)
{
    int32_t status;

    (void)uart_ctrl_init();
    printf("\n========================================\n");
    printf(" CPHMC J721E Cortex-A72_0 BareMetal\n");
    printf("========================================\n");

    status = gpio_ctrl_init();
    if (status != 0)
    {
        printf("GPIO init failed, status=%d\n", status);
        return status;
    }
    GPIO_write(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE0_PIN, 0U);
    GPIO_write(GPIO_CTRL_PORT, GPIO_CTRL_BOOTMODE2_PIN, 0U);

    status = gpio_intr_init();
    if (status != 0)
    {
        return status;
    }

    status = udma_ctrl_init();
    if (status == 0)
    {
        status = udma_ctrl_self_test();
    }
    printf("UDMA MAIN0 block-copy: %s, status=%d\n",
           (status == 0) ? "PASS" : "FAIL", status);

    g_ipcStatus = ipc_ctrl_init();
    if (g_ipcStatus == 0)
    {
        printf("IPC A72<->C7X: READY (TI IPC/RPMessage)\n");
        g_ipcStatus = ipc_ctrl_ping();
        printf("IPC A72->C7X: ping %s (endpoint 15 -> 14)\n",
               (g_ipcStatus == 0) ? "sent" : "failed");
    }
    else
    {
        printf("IPC A72<->C7X: INIT FAIL, status=%d\n", g_ipcStatus);
    }

    status = interrupt_ctrl_init();
    if (status != 0)
    {
        printf("interrupt init failed, status=%d\n", status);
        return status;
    }
    status = app_register_tasks();
    if (status != 0)
    {
        printf("application interrupt registration failed, status=%d\n", status);
        return status;
    }

    printf("Interrupt architecture: hard timer -> soft IRQ -> app HWI/loop tasks\n");
    interrupt_ctrl_enable();
    return 0;
}

void app_main_run(void)
{
    while (1)
    {
        app_run_loop_tasks();
    }
}
