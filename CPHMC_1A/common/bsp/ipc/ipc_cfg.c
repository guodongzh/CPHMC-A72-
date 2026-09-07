/**
 *  \file ex02_bios_multicore_echo_test.c
 *
 *  \brief Multi-core (BIOS-to-BIOS) IPC echo test application performing basic echo
 *  communication using the IPC driver
 *
 */

#include "ipc_cfg.h"
#include "ti/osal/osal.h"

#if defined(BUILD_MCU)
char Ipc_traceBuffer[IPC_TRACE_BUFFER_MAX_SIZE] __attribute__((section(".tracebuf")));
#else
char Ipc_traceBuffer[IPC_TRACE_BUFFER_MAX_SIZE];
#endif

const Ipc_ResourceTable ti_ipc_remoteproc_ResourceTable
    __attribute__((section(".resource_table"), used, aligned(4096))) = {
        .base =
            {
                .ver = 1U,          /* we're the first version that implements this */
                .num = NUM_ENTRIES, /* number of entries in the table */
                .reserved[0] = 0U,
                .reserved[1] = 0U, /* reserved, must be zero */
            },

        /* offsets to entries */
        .offset =
            {
                offsetof(Ipc_ResourceTable, rpmsg_vdev),
                offsetof(Ipc_ResourceTable, trace),
            },

        /* rpmsg vdev entry */
        .rpmsg_vdev =
            {
                .type = TYPE_VDEV,
                .id = VIRTIO_ID_RPMSG,
                .notifyid = 0U,
#if defined(BUILD_C66X)
                .dfeatures = RPMSG_C66_DSP_FEATURES,
                .gfeatures = 0U,
                .config_len = 0U,
                .status = 0U,
                .num_of_vrings = 2U,
                .reserved = {0U, 0U},
#elif defined(BUILD_C7X)
                .dfeatures = RPMSG_C7X_DSP_FEATURES,
                .gfeatures = 0U,
                .config_len = 0U,
                .status = 0U,
                .num_of_vrings = 2U,
                .reserved = {0U, 0U},
#else
                .dfeatures = RPMSG_R5F_C0_FEATURES,
                .gfeatures = 0U,
                .config_len = 0U,
                .status = 0U,
                .num_of_vrings = 2U,
                .reserved = {0U, 0U},
#endif
            },
/* the two vrings */
#if defined(BUILD_MCU1_0)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_MCU1_1)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_MCU2_0)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_MCU2_1)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_MCU3_0)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_MCU3_1)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, R5F_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_C66X_1)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, C66_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, C66_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_C66X_2)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, C66_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, C66_RPMSG_VQ1_SIZE, 2U, 0U},
#elif defined(BUILD_C7X_1)
        .rpmsg_vring0 = {RPMSG_VRING_ADDR_ANY, 4096U, C7X_RPMSG_VQ0_SIZE, 1U, 0U},
        .rpmsg_vring1 = {RPMSG_VRING_ADDR_ANY, 4096U, C7X_RPMSG_VQ1_SIZE, 2U, 0U},
#endif

        .trace =
            {
#ifdef BUILD_C7X
                .type = (TRACE_INTS_VER1 | TYPE_TRACE),
                .da = (uint64_t)&Ipc_traceBuffer,
                .len = IPC_TRACE_BUFFER_MAX_SIZE,
                .reserved = 0,
                .name = "trace:r5f0",
#else
                .type = (TRACE_INTS_VER0 | TYPE_TRACE),
                .da = (uint32_t)&Ipc_traceBuffer,
                .len = IPC_TRACE_BUFFER_MAX_SIZE,
                .reserved = 0,
                .name = "trace:r5f0",
#endif
            },
};

/*
 * In the cfg file of R5F, C66x, default heap is 48K which is not
 * enough for 9 task_stack, so creating task_stack on global.
 * C7x cfg has 256k default heap, so no need to put task_stack on global
 */

#if !defined(BUILD_C7X)

uint8_t gCtrlTaskBuf[IPC_TASK_STACKSIZE] __attribute__((aligned(4)));

#else

/* IMPORTANT NOTE: For C7x,
 * - stack size and stack ptr MUST be 8KB aligned
 * - AND min stack size MUST be 16KB
 * - AND stack assigned for task context is "size - 8KB"
 *       - 8KB chunk for the stack area is used for interrupt handling in this task context
 */
uint8_t gCtrlTaskBuf[IPC_TASK_STACKSIZE] __attribute__((section(".bss:taskStackSection")))
__attribute__((aligned(8192)));
#endif

uint8_t gCntrlBuf[RPMSG_DATA_SIZE] __attribute__((section(".ipc_data_buffer"), aligned(8)));
uint8_t gSysVqBuf[VQ_BUF_SIZE] __attribute__((section(".ipc_data_buffer"), aligned(8)));

#ifdef BUILD_MCU1_0
uint32_t selfProcId = IPC_MCU1_0;
uint32_t remoteProc[] = {
#if defined(SOC_AM65XX)
    IPC_MPU1_0,
    IPC_MCU1_1
#elif defined(SOC_J721E)
    IPC_MPU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C66X_1,
    IPC_C66X_2,
    IPC_C7X_1
#elif defined(SOC_J7200)
    IPC_MPU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1
#elif defined(SOC_AM64X)
    IPC_MPU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_M4F_0
#elif defined(SOC_J721S2)
    IPC_MPU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C7X_1,
    IPC_C7X_2
#elif defined(SOC_J784S4)
    IPC_MPU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_MCU4_0,
    IPC_MCU4_1,
    IPC_C7X_1,
    IPC_C7X_2,
    IPC_C7X_3,
    IPC_C7X_4
#endif
};
#endif

#ifdef BUILD_MCU1_1
uint32_t selfProcId = IPC_MCU1_1;
uint32_t remoteProc[] = {
#if defined(SOC_AM65XX)
    IPC_MPU1_0,
    IPC_MCU1_0
#elif defined(SOC_J721E)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C66X_1,
    IPC_C66X_2,
    IPC_C7X_1
#elif defined(SOC_J7200)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU2_0,
    IPC_MCU2_1
#elif defined(SOC_AM64X)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_M4F_0
#elif defined(SOC_J721S2)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C7X_1,
    IPC_C7X_2
#elif defined(SOC_J784S4)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_MCU4_0,
    IPC_MCU4_1,
    IPC_C7X_1,
    IPC_C7X_2,
    IPC_C7X_3,
    IPC_C7X_4
#endif
};
#endif

#ifdef BUILD_MCU2_0
uint32_t selfProcId = IPC_MCU2_0;
uint32_t remoteProc[] = {
#if defined(SOC_J721E)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C66X_1,
    IPC_C66X_2,
    IPC_C7X_1
#elif defined(SOC_J7200)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_1
#elif defined(SOC_AM64X)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_1,
    IPC_M4F_0
#elif defined(SOC_J721S2)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C7X_1,
    IPC_C7X_2
#elif defined(SOC_J784S4)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_MCU4_0,
    IPC_MCU4_1,
    IPC_C7X_1,
    IPC_C7X_2,
    IPC_C7X_3,
    IPC_C7X_4
#endif
};
#endif

#ifdef BUILD_MCU2_1
uint32_t selfProcId = IPC_MCU2_1;
uint32_t remoteProc[] = {
#if defined(SOC_J721E)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C66X_1,
    IPC_C66X_2,
    IPC_C7X_1
#elif defined(SOC_J7200)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0
#elif defined(SOC_AM64X)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_M4F_0
#elif defined(SOC_J721S2)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C7X_1,
    IPC_C7X_2
#elif defined(SOC_J784S4)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_MCU4_0,
    IPC_MCU4_1,
    IPC_C7X_1,
    IPC_C7X_2,
    IPC_C7X_3,
    IPC_C7X_4
#endif
};
#endif

#ifdef BUILD_MCU3_0
uint32_t selfProcId = IPC_MCU3_0;
uint32_t remoteProc[] = {
#if defined(SOC_J721E)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_1,
    IPC_C66X_1,
    IPC_C66X_2,
    IPC_C7X_1
#elif defined(SOC_J721S2)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_1,
    IPC_C7X_1,
    IPC_C7X_2
#elif defined(SOC_J784S4)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_1,
    IPC_MCU4_0,
    IPC_MCU4_1,
    IPC_C7X_1,
    IPC_C7X_2,
    IPC_C7X_3,
    IPC_C7X_4
#endif
};
#endif

#ifdef BUILD_MCU3_1
uint32_t selfProcId = IPC_MCU3_1;
uint32_t remoteProc[] = {
#if defined(SOC_J721E)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_C66X_1,
    IPC_C66X_2,
    IPC_C7X_1
#elif defined(SOC_J721S2)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_C7X_1,
    IPC_C7X_2
#elif defined(SOC_J784S4)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU4_0,
    IPC_MCU4_1,
    IPC_C7X_1,
    IPC_C7X_2,
    IPC_C7X_3,
    IPC_C7X_4
#endif
};
#endif

#ifdef BUILD_C66X_1
uint32_t selfProcId = IPC_C66X_1;
uint32_t remoteProc[] = {
#ifndef NO_R5FS
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C66X_2,
    IPC_C7X_1
#else
    IPC_MPU1_0,
    IPC_C66X_2,
    IPC_C7X_1
#endif
};
#endif

#ifdef BUILD_C66X_2
uint32_t selfProcId = IPC_C66X_2;
uint32_t remoteProc[] = {IPC_MPU1_0,
                         IPC_MCU1_0,
                         IPC_MCU1_1,
                         IPC_MCU2_0,
                         IPC_MCU2_1,
                         IPC_MCU3_0,
                         IPC_MCU3_1,
                         IPC_C66X_1,
                         IPC_C7X_1};
#endif

#ifdef BUILD_C7X_1
uint32_t selfProcId = IPC_C7X_1;
uint32_t remoteProc[] = {
#if defined(SOC_J721E)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C66X_1,
    IPC_C66X_2
#elif defined(SOC_J721S2)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_C7X_2
#elif defined(SOC_J784S4)
    IPC_MPU1_0,
    IPC_MCU1_0,
    IPC_MCU1_1,
    IPC_MCU2_0,
    IPC_MCU2_1,
    IPC_MCU3_0,
    IPC_MCU3_1,
    IPC_MCU4_0,
    IPC_MCU4_1,
    IPC_C7X_2,
    IPC_C7X_3,
    IPC_C7X_4
#endif
};
#endif

uint32_t *pRemoteProcArray = remoteProc;
uint32_t gNumRemoteProc = sizeof(remoteProc) / sizeof(uint32_t);

static inline void IpcTestPrint(const char *str) { IPC_log("%s", str); }
int ipc_try_cnt = 1;
void ipc_lib_init(void)
{
    uint32_t numProc = gNumRemoteProc;
    Ipc_VirtIoParams vqParam;
    Ipc_InitPrms initPrms;

    /* Step1 : Initialize the multiproc */
    Ipc_mpSetConfig(selfProcId, numProc, pRemoteProcArray);

    IPC_log("ipc_lib_init (core : %s) .....\r\n", Ipc_mpGetSelfName());

    /* Initialize params with defaults */
    IpcInitPrms_init(0U, &initPrms);
    initPrms.printFxn = &IpcTestPrint;
    Ipc_init(&initPrms);

    IPC_log("Required Local memory for Virtio_Object = %d\r\n",
            numProc * Ipc_getVqObjMemoryRequiredPerCore());

    /* If A72 remote core is running Linux OS, then
     * load resource table
     */
    Ipc_loadResourceTable((void *)&ti_ipc_remoteproc_ResourceTable);

    /* Wait for Linux VDev ready... */
    while (ipc_try_cnt)
    {
        if (Ipc_isRemoteReady(IPC_MPU1_0))
        {
            break;
        }
        Osal_delay(1000);
    }
    if (ipc_try_cnt == 0)
    {
        IPC_log("Linux VDEV not ready\n");
    }
    else
    {
        IPC_log("Linux VDEV ready now .....\n");
    }

    /* Step2 : Initialize Virtio */
    vqParam.vqObjBaseAddr = (void *)gSysVqBuf;
    vqParam.vqBufSize = numProc * Ipc_getVqObjMemoryRequiredPerCore();
    vqParam.vringBaseAddr = (void *)VRING_BASE_ADDRESS;
    vqParam.vringBufSize = IPC_VRING_BUFFER_SIZE;
    vqParam.timeoutCnt = 100; /* Wait for counts */
    Ipc_initVirtIO(&vqParam);

    /* Step 3: Initialize RPMessage Module*/
    RPMessage_Params cntrlParam;

    IPC_log("Required Local memory for RPMessage Object = %d\n", RPMessage_getObjMemRequired());

    /* Initialize the param */
    RPMessageParams_init(&cntrlParam);

    /* Set memory for HeapMemory for control task */
    cntrlParam.buf = gCntrlBuf;
    cntrlParam.bufSize = sizeof(gCntrlBuf);
    cntrlParam.stackBuffer = gCtrlTaskBuf;
    cntrlParam.stackSize = sizeof(gCtrlTaskBuf);
    RPMessage_init(&cntrlParam);
}
