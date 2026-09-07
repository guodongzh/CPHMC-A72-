#include <stdio.h>
#include <string.h>

#include "debug_config.h"
#include "ipc_test.h"
#include <ti/drv/ipc/ipc.h>
#include <ti/drv/uart/UART_stdio.h>
#include <ti/osal/osal.h>

#define MSGSIZE        256U
#define SERVICE_PING   "ti.ipc4.ping-pong"
#define ENDPT_PING     13U
#define SERVICE_CHRDEV "rpmsg_chrdev"
#define ENDPT_CHRDEV   14U

uint8_t g_sendBuf[RPMSG_DATA_SIZE] __attribute__((section(".ipc_data_buffer"), aligned(8)));
uint8_t g_rspBuf[RPMSG_DATA_SIZE] __attribute__((section(".ipc_data_buffer"), aligned(8)));
uint8_t g_rspBufLinux[RPMSG_DATA_SIZE] __attribute__((section(".ipc_data_buffer"), aligned(8)));
uint8_t gSendTaskStack[IPC_TASK_STACKSIZE] __attribute__((aligned(4)));
uint8_t gRspTaskStack[IPC_TASK_STACKSIZE] __attribute__((aligned(4)));


/**
 * This "Task" waits for a "ping" message from any processor
 * then replies with a "pong" message.
 */
void rpmsg_responder_fxn(void *arg0, void *arg1)
{
    RPMessage_Handle handle;
    RPMessage_Params params;
    uint32_t myEndPt = 0;
    uint32_t remoteEndPt;
    uint32_t remoteProcId;
    uint16_t len;
    int32_t n;
    int32_t status = 0;
    char *name     = SERVICE_PING;
    uintptr_t key;

    char str[MSGSIZE];

    IPC_log("RecvTask Start!!\n");

    RPMessageParams_init(&params);
    params.requestedEndpt = ENDPT_PING;
    params.buf            = g_rspBuf;
    params.bufSize        = sizeof(g_rspBuf);
    handle                = RPMessage_create(&params, &myEndPt);
    if (!handle)
    {
        IPC_log("RecvTask: Failed to create endpoint\n");
        return;
    }
    else
    {
        IPC_log("RecvTask: create endpoint %d\n", myEndPt);
    }

    status = RPMessage_announce(RPMESSAGE_ALL, myEndPt, name);
    if (status != IPC_SOK)
    {
        IPC_log("RecvTask: RPMessage_announce() for %s failed\n", name);
        return;
    }

    while (1)
    {
        status = RPMessage_recv(handle, (Ptr)str, &len, &remoteEndPt, &remoteProcId,
                                IPC_RPMESSAGE_TIMEOUT_FOREVER);
        if (status != IPC_SOK)
        {
            IPC_log("RecvTask: failed with code %d\n", status);
        }
        else
        {
            /* NULL terminated string */
            str[len] = '\0';
            IPC_log("RecvTask: Revcvd msg \"%s\" len %d from %s\n",
                    str, len, Ipc_mpGetName(remoteProcId));
        }

        status = sscanf(str, "ping %d", &n);
        if (status == 1)
        {
            memset(str, 0, MSGSIZE);
            /* Not having a Hwip_disable here causes C7X goes into a bad state. This is for SafeRTOS on C7X. */
            key = HwiP_disable();
            len = snprintf(str, 255, "pong %d", n);
            HwiP_restore(key);
            if (len > 255)
            {
                IPC_log("RecvTask: snprintf failed, len %d\n", len);
                len = 255;
            }
            str[len++] = '\0';
        }
        else
        {
            /* print the message */
            IPC_log("%s <--> %s :  \"%s\" recvd\n",
                    Ipc_mpGetSelfName(), Ipc_mpGetName(remoteProcId), str);
        }

        IPC_log("RecvTask: Sending msg \"%s\" len %d from %s to %s\n",
                str, len, Ipc_mpGetSelfName(), Ipc_mpGetName(remoteProcId));

        status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, str, len);
        if (status != IPC_SOK)
        {
            IPC_log("RecvTask: Sending msg \"%s\" len %d from %s to %s failed!!!\n",
                    str, len, Ipc_mpGetSelfName(), Ipc_mpGetName(remoteProcId));
        }
    }
}

void rpmsg_sender_fxn(void *arg0, void *arg1)
{
    RPMessage_Handle handle;
    RPMessage_Params params;
    uint32_t myEndPt = 0;
    uint32_t remoteEndPt;
    uint32_t remoteProcId;
    uint32_t dstProc;
    uint32_t dstProcList[] = {IPC_MCU2_1, IPC_MCU3_0, IPC_MCU3_1, IPC_C66X_1, IPC_C66X_2, IPC_C7X_1};
    uint16_t len;
    int32_t status;
    char buf[256], str[256];
    uintptr_t key;
    uint32_t cntPing = 0;
    uint32_t cntPong = 0;

    dstProc = IPC_C66X_1;

    /* Create the endpoint for sending. */
    RPMessageParams_init(&params);
    params.numBufs = 2;
    params.buf     = g_sendBuf;
    params.bufSize = sizeof(g_sendBuf);
    handle         = RPMessage_create(&params, &myEndPt);
    if (!handle)
    {
        IPC_log("SendTask%d: Failed to create message endpoint\n",
                dstProc);
        return;
    }

    for (int i = 0; i < sizeof(dstProcList) / sizeof(uint32_t); ++i)
    {
        status = RPMessage_getRemoteEndPt(dstProcList[i], SERVICE_PING, &remoteProcId,
                                          &remoteEndPt, SemaphoreP_WAIT_FOREVER);

        IPC_log("%s: RPMessage_getRemoteEndPt(%d) = %d\n", __func__, dstProcList[i], status);
    }

    IPC_log("%s: ready for send msg\n", __func__);
    /* Send data to remote endPt: */
    while (1)
    {
        memset(str, 0, 256);
        UART_scanFmt("%s", str);
        if (str[0] == '0')
        {
            break;
        }

        memset(buf, 0, 256);
        /* Not having a Hwip_disable here causes C7X go into a bad state. This is for SafeRTOS on C7X */
        key = HwiP_disable();
        len = snprintf(buf, 255, "%s", str);
        HwiP_restore(key);
        if (len > 255)
        {
            IPC_log("SendTask%d: snprintf failed, len %d\n", dstProc, len);
            len = 255;
        }
        buf[len++] = '\0';

        /* Increase the Ping Counter */
        cntPing++;

        for (int i = 0; i < sizeof(dstProcList) / sizeof(uint32_t); ++i)
        {
            IPC_log("%s: Sending \"%s\" from %s to %s...\n",
                    __func__, buf, Ipc_mpGetSelfName(), Ipc_mpGetName(dstProcList[i]));
            status = RPMessage_send(handle, dstProcList[i], ENDPT_PING, myEndPt, (Ptr)buf, len);
            if (status != IPC_SOK)
            {
                IPC_log("%s: RPMessage_send Failed Msg-> \"%s\" from %s to %s...\n",
                        __func__, buf, Ipc_mpGetSelfName(), Ipc_mpGetName(dstProcList[i]));
                break;
            }
            /* wait for a response message: */
            RPMessage_recv(handle, (Ptr)buf, &len, &remoteEndPt,
                           &remoteProcId, IPC_RPMESSAGE_TIMEOUT_FOREVER);
            IPC_log("%s: recv \"%s\" from %s \n", __func__, buf, Ipc_mpGetName(remoteProcId));
        }
        IPC_log("%s: recv all msg\n", __func__);

        if (status != IPC_SOK)
        {
            IPC_log("SendTask%d: RPMessage_recv failed with code %d\n",
                    dstProc, status);
            break;
        }

        /* Make it NULL terminated string */
        if (len >= MSGSIZE)
        {
            buf[MSGSIZE - 1] = '\0';
        }
        else
        {
            buf[len] = '\0';
        }

#ifdef DEBUG_PRINT
        IPC_log("SendTask%d: Received \"%s\" len %d from %s endPt %d \n",
                dstProc, buf, len, Ipc_mpGetName(remoteProcId), remoteEndPt);
#endif
        cntPong++;
    }

    IPC_log("%s <--> %s, Ping- %d, pong - %d completed\n",
            Ipc_mpGetSelfName(), Ipc_mpGetName(dstProc), cntPing, cntPong);

    /* Delete the RPMesg object now */
    RPMessage_delete(&handle);
}

/*
 * This "Task" waits for a "ping" message from any processor
 * then replies with a "pong" message.
 */
void rpmsg_responde_linuxr_fxn()
{
    RPMessage_Handle handle;
    RPMessage_Params params;
    uint32_t myEndPt = 0;
    uint32_t remoteEndPt;
    uint32_t remoteProcId;
    uint16_t len;
    int32_t n;
    int32_t status;
    uintptr_t key;
    char str[MSGSIZE];

    RPMessageParams_init(&params);
    params.requestedEndpt = ENDPT_CHRDEV;
    params.buf            = g_rspBufLinux;
    params.bufSize        = sizeof(g_rspBufLinux);

    handle = RPMessage_create(&params, &myEndPt);
    if (!handle)
    {
        IPC_log("RecvTask: Failed to create endpoint\n");
        return;
    }

    status = RPMessage_announce(IPC_MPU1_0, myEndPt, SERVICE_CHRDEV);
    if (status != IPC_SOK)
    {
        IPC_log("RecvTask: RPMessage_announce() for %s failed\n", SERVICE_CHRDEV);
        return;
    }

    while (1)
    {
        status = RPMessage_recv(handle, (Ptr)str, &len, &remoteEndPt, &remoteProcId,
                                IPC_RPMESSAGE_TIMEOUT_FOREVER);

        if (status != IPC_SOK)
        {
            IPC_log("RecvTask: failed with code %d\n", status);
        }
        else
        {
            /* NULL terminated string */
            str[len] = '\0';
#ifdef DEBUG_PRINT
            IPC_log("RecvTask: Revcvd msg \"%s\" len %d from %s\n",
                    str, len, Ipc_mpGetName(remoteProcId));
#endif
        }

        status = sscanf(str, "ping %d", &n);
        if (status == 1)
        {
            memset(str, 0, MSGSIZE);
            /* Not having a Hwip_disable here causes C7X goes into a bad state. This is for SafeRTOS on C7X. */
            key = HwiP_disable();
            len = snprintf(str, 255, "pong %d", n);
            HwiP_restore(key);
            if (len > 255)
            {
                IPC_log("RecvTask: snprintf failed, len %d\n", len);
                len = 255;
            }
            str[len++] = '\0';
        }
        else
        {
            /* If this is not ping/pong message, just print the message */
            IPC_log("%s <--> %s : %s recvd\n",
                    Ipc_mpGetSelfName(),Ipc_mpGetName(remoteProcId), str);
        }
#ifdef DEBUG_PRINT
        IPC_log("RecvTask: Sending msg \"%s\" len %d from %s to %s\n",
                str, len, Ipc_mpGetSelfName(),
                Ipc_mpGetName(remoteProcId));
#endif
        status = RPMessage_send(handle, remoteProcId, remoteEndPt, myEndPt, str, len);
        if (status != IPC_SOK)
        {
            IPC_log("RecvTask: Sending msg \"%s\" len %d from %s to %s failed!!!\n",
                    str, len, Ipc_mpGetSelfName(),
                    Ipc_mpGetName(remoteProcId));
        }
    }

#if (__ARM_ARCH_PROFILE == 'R') || (__ARM_ARCH_PROFILE == 'M')
    /* For ARM R and M cores*/
    __asm__ __volatile__("wfi"
                         "\n\t" : : : "memory");
#endif

#if defined(BUILD_C66X)
    __asm(" IDLE");
#elif defined(BUILD_C7X)
    __asm(" IDLE");
#endif
}

void ipc_test(void)
{
    rpmsg_responde_linuxr_fxn();
}
