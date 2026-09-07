#ifndef IPC_CFG_H
#define IPC_CFG_H

#include "debug_config.h"
#include <ti/drv/ipc/ipc.h>
#include <stdint.h>
#include <stddef.h>
#include <ti/drv/ipc/include/ipc_rsctypes.h>
#include <ti/osal/TaskP.h>
#include "memory_map_defines.h"

/* this should be >= RPMessage_getObjMemRequired() */
#define IPC_RPMESSAGE_OBJ_SIZE 256U

/* this should be >= RPMessage_getMessageBufferSize() */
#define IPC_RPMESSAGE_MSG_BUFFER_SIZE (496U + 32U)

#define RPMSG_DATA_SIZE               (256U * IPC_RPMESSAGE_MSG_BUFFER_SIZE + IPC_RPMESSAGE_OBJ_SIZE)
#define VQ_BUF_SIZE                   2048U

/* VRING base address, all VRINGs are put one after other in the below region.
 *
 * IMPORTANT: Make sure of below,
 * - The section defined below should be placed at the exact same location in memory for all the CPUs
 * - The memory should be marked as non-cached for all the CPUs
 */
//#define VRING_BASE_ADDRESS 0xAA000000U
#define VRING_BASE_ADDRESS IPC_VRING_SPACE_START

/*
 * Sizes of the virtqueues (expressed in number of buffers supported,
 * and must be power of 2)
 */
#define R5F_RPMSG_VQ0_SIZE 256U
#define R5F_RPMSG_VQ1_SIZE 256U
#define C66_RPMSG_VQ0_SIZE 256U
#define C66_RPMSG_VQ1_SIZE 256U
#define C7X_RPMSG_VQ0_SIZE 256U
#define C7X_RPMSG_VQ1_SIZE 256U

/* flip up bits whose indices represent features we support */
#define RPMSG_R5F_C0_FEATURES     1U
#define RPMSG_C66_DSP_FEATURES    1U
#define RPMSG_C7X_DSP_FEATURES    1U

#define IPC_TRACE_BUFFER_MAX_SIZE (0x80000)

#define RPMSG_VRING_ADDR_ANY FW_RSC_ADDR_ANY

void ipc_lib_init(void);

#endif // IPC_CFG_H
