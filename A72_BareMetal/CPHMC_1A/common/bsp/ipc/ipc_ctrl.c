/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       ipc_ctrl.c
 *@author     zhaoguodong
 *@date       2026.09.11
 *@brief      TI IPC RPMessage wrapper for the A72 to C7X channel.
 *@par        History
 *Date        Version   Author       Description
 *2026.09.11  1.0       zhaoguodong  Create file.
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "ipc_ctrl.h"

#include <stddef.h>
#include <stdint.h>

#include "memory_map_defines.h"

#include <ti/drv/ipc/ipc.h>
#include <ti/drv/ipc/include/ipc_mp.h>
#include <ti/drv/ipc/include/ipc_virtio.h>
#include <ti/drv/uart/UART_stdio.h>

/*---------------------------------------------------------------------------*/
/*                             Local Macros                                  */
/*---------------------------------------------------------------------------*/
#define IPC_C7X_REMOTE_PROC_ID       (IPC_C7X_1)
#define IPC_A72_ENDPOINT              (15U)
#define IPC_C7X_ENDPOINT              (14U)
#define IPC_VQ_OBJECT_BUFFER_SIZE     (0x00000800UL)
#define IPC_RPMESSAGE_BUFFER_SIZE     (0x00021000UL)
#define IPC_PING_LENGTH                (6U)

#if (IPC_VRING_SPACE_START != 0xAA000000U)
#error "A72 and C7X IPC VRING memory maps are inconsistent"
#endif

/*---------------------------------------------------------------------------*/
/*                            Global Variables                               */
/*---------------------------------------------------------------------------*/
static uint32_t gIpcRemoteProc[] = {IPC_C7X_REMOTE_PROC_ID};

static uint8_t gIpcVqObjectBuffer[IPC_VQ_OBJECT_BUFFER_SIZE]
    __attribute__((aligned(8)));
static uint8_t gIpcCtrlBuffer[IPC_RPMESSAGE_BUFFER_SIZE]
    __attribute__((aligned(8)));
static uint8_t gIpcRxBuffer[IPC_RPMESSAGE_BUFFER_SIZE]
    __attribute__((aligned(8)));

static RPMessage_Handle gIpcHandle;
static uint32_t gIpcLocalEndpoint;
static volatile uint32_t gIpcRxCount;
static volatile uint32_t gIpcReplyCount;
static volatile uint32_t gIpcMessageCount;
static uint8_t gIpcInitialized;

/*---------------------------------------------------------------------------*/
/*                          Function Declarations                            */
/*---------------------------------------------------------------------------*/
static uint32_t ipc_ctrl_virt_to_phy(const void *virtAddr);
static void *ipc_ctrl_phy_to_virt(uint32_t phyAddr);
static void ipc_ctrl_print(const char *str);
static void ipc_ctrl_new_message(uint32_t srcEndPt, uint32_t procId);
static uint8_t ipc_ctrl_is_pong(const uint8_t *message, uint16_t length);

/*---------------------------------------------------------------------------*/
/*                          Function Definitions                             */
/*---------------------------------------------------------------------------*/
static uint32_t ipc_ctrl_virt_to_phy(const void *virtAddr)
{
    return (uint32_t)(uintptr_t)virtAddr;
}

static void *ipc_ctrl_phy_to_virt(uint32_t phyAddr)
{
    return (void *)(uintptr_t)phyAddr;
}

static void ipc_ctrl_print(const char *str)
{
    UART_printf("%s", str);
}

static void ipc_ctrl_new_message(uint32_t srcEndPt, uint32_t procId)
{
    (void)srcEndPt;
    (void)procId;
    gIpcRxCount++;
}

static uint8_t ipc_ctrl_is_pong(const uint8_t *message, uint16_t length)
{
    static const uint8_t pongMessage[] = "pong 1";
    uint32_t index;

    if (length != IPC_PING_LENGTH)
    {
        return 0U;
    }

    for (index = 0U; index < IPC_PING_LENGTH; index++)
    {
        if (message[index] != pongMessage[index])
        {
            return 0U;
        }
    }

    return 1U;
}

int32_t ipc_ctrl_init(void)
{
    int32_t status;
    Ipc_InitPrms initPrms;
    Ipc_VirtIoParams virtIoPrms;
    RPMessage_Params messagePrms;

    if (gIpcInitialized != 0U)
    {
        return 0;
    }

    status = Ipc_mpSetConfig(IPC_MPU1_0, 1U, gIpcRemoteProc);
    if (status != IPC_SOK)
    {
        return status;
    }

    IpcInitPrms_init(0U, &initPrms);
    initPrms.newMsgFxn = ipc_ctrl_new_message;
    initPrms.virtToPhyFxn = ipc_ctrl_virt_to_phy;
    initPrms.phyToVirtFxn = ipc_ctrl_phy_to_virt;
    initPrms.printFxn = ipc_ctrl_print;

    status = Ipc_init(&initPrms);
    if (status != IPC_SOK)
    {
        return status;
    }

    virtIoPrms.vqObjBaseAddr = gIpcVqObjectBuffer;
    virtIoPrms.vqBufSize = sizeof(gIpcVqObjectBuffer);
    virtIoPrms.vringBaseAddr = (void *)(uintptr_t)IPC_VRING_SPACE_START;
    virtIoPrms.vringBufSize = IPC_VRING_SPACE_SIZE;
    virtIoPrms.timeoutCnt = 100U;
    status = Ipc_initVirtIO(&virtIoPrms);
    if (status != IPC_SOK)
    {
        return status;
    }

    status = RPMessageParams_init(&messagePrms);
    if (status != IPC_SOK)
    {
        return status;
    }

    messagePrms.buf = gIpcCtrlBuffer;
    messagePrms.bufSize = sizeof(gIpcCtrlBuffer);
    messagePrms.stackBuffer = NULL;
    messagePrms.stackSize = 0U;
    status = RPMessage_init(&messagePrms);
    if (status != IPC_SOK)
    {
        return status;
    }

    status = RPMessageParams_init(&messagePrms);
    if (status != IPC_SOK)
    {
        return status;
    }

    messagePrms.requestedEndpt = IPC_A72_ENDPOINT;
    messagePrms.buf = gIpcRxBuffer;
    messagePrms.bufSize = sizeof(gIpcRxBuffer);
    gIpcHandle = RPMessage_create(&messagePrms, &gIpcLocalEndpoint);
    if (gIpcHandle == NULL)
    {
        return IPC_EFAIL;
    }

    gIpcInitialized = 1U;
    return 0;
}

int32_t ipc_ctrl_ping(void)
{
    static uint8_t pingMessage[] = "ping 1";

    if (gIpcInitialized == 0U)
    {
        return IPC_EFAIL;
    }

    return RPMessage_send(gIpcHandle,
                          IPC_C7X_REMOTE_PROC_ID,
                          IPC_C7X_ENDPOINT,
                          gIpcLocalEndpoint,
                          pingMessage,
                          IPC_PING_LENGTH);
}

int32_t ipc_ctrl_poll(void)
{
    int32_t status;
    uint8_t message[32U];
    uint16_t length = 0U;
    uint32_t remoteEndpoint = 0U;
    uint32_t remoteProcId = 0U;

    if (gIpcInitialized == 0U)
    {
        return IPC_EFAIL;
    }

    status = RPMessage_recvNb(gIpcHandle,
                              message,
                              &length,
                              &remoteEndpoint,
                              &remoteProcId);
    if (status == IPC_SOK)
    {
        gIpcMessageCount++;
        if (ipc_ctrl_is_pong(message, length) != 0U)
        {
            gIpcReplyCount++;
        }
    }

    return status;
}

uint32_t ipc_ctrl_get_rx_count(void)
{
    (void)gIpcRxCount;
    return gIpcReplyCount;
}

uint32_t ipc_ctrl_get_message_count(void)
{
    return gIpcMessageCount;
}
