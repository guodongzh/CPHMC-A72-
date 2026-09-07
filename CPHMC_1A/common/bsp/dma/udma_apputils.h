/*
 *  Copyright (c) Texas Instruments Incorporated 2018-2023
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 *  \file udma_apputils.h
 *
 *  \brief Common UDMA application utility used in all UDMA example.
 *
 *  NOTE: This library is meant only for UDMA examples. Customers are not
 *  encouraged to use this layer as these are very specific to the examples
 *  written and the API behaviour and signature can change at any time to
 *  suit the examples.
 *
 */

#ifndef UDMA_APPUTILS_H_
#define UDMA_APPUTILS_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "ti/csl/csl_udmap.h"
#include "ti/drv/udma/udma.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* None */

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/**
 *  \brief Virtual to physical translation function.
 *
 *  \param virtAddr [IN] Virtual address
 *  \return Corresponding physical address
 */
uint64_t Udma_appVirtToPhyFxn(const void *virtAddr);

/**
 *  \brief Physical to virtual translation function.
 *
 *  \param phyAddr  [IN] Physical address
 *  \return Corresponding virtual address
 */
void *Udma_appPhyToVirtFxn(uint64_t phyAddr);

void UDMA_Hpd_Init(Udma_ChHandle chHandle,
                   CSL_UdmapCppi5HMPD *pHpd,
                   uint64_t bufPtr,
                   uint32_t length);

extern struct Udma_DrvObj gUdmaDrvObj;

int32_t UDMA_Lib_Init();

#ifdef __cplusplus
}
#endif

#endif  /* #define UDMA_APPUTILS_H_ */
