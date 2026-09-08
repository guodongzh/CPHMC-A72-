/*
 * Copyright (C) 2023 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "ti/osal/src/printf.h"
#include "sbl_qos.h"

static void qos_mmr_write(uint32_t addr, uint32_t value)
{
    CSL_REG32_WR(addr, value);
    /* J721E TRM requires a readback for writes in 0x45000000-0x45ffffff. */
    (void)CSL_REG32_RD(addr);
}


void setup_navss_nb(void)
{
    /* Map orderid 8-15 to VBUSM.C thread 2 (real-time traffic) */
    CSL_REG32_WR(NAVSS0_NBSS_NB0_CFG_NB_THREADMAP, 2);
    CSL_REG32_WR(NAVSS0_NBSS_NB1_CFG_NB_THREADMAP, 2);
}

void setup_vpac_qos(void)
{
    unsigned int channel, group;

    /* vpac data master 0  */
    for (channel = 0; channel < QOS_VPAC0_DATA0_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_VPAC0_DATA0_CBASS_MAP(channel), (QOS_VPAC0_DATA0_ATYPE << 28));
    }

    /* vpac data master 1  */
    for (channel = 0; channel < QOS_VPAC0_DATA1_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_VPAC0_DATA1_CBASS_MAP(channel), (QOS_VPAC0_DATA1_ATYPE << 28));
    }

    /* vpac ldc0  */
    for (group = 0; group < QOS_VPAC0_LDC0_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_VPAC0_LDC0_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_VPAC0_LDC0_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_VPAC0_LDC0_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_VPAC0_LDC0_CBASS_MAP(channel), (QOS_VPAC0_LDC0_ATYPE << 28) | (QOS_VPAC0_LDC0_PRIORITY << 12) | (QOS_VPAC0_LDC0_ORDER_ID << 4));
    }

}

void setup_dmpac_qos(void)
{
    unsigned int channel;

    /* dmpac data  */
    for (channel = 0; channel < QOS_DMPAC0_DATA_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_DMPAC0_DATA_CBASS_MAP(channel), (QOS_DMPAC0_DATA_ATYPE << 28));
    }
}

void setup_dss_qos(void)
{
    unsigned int channel, group;

    /* two master ports: dma and fbdc */
    /* two groups: SRAM and DDR */
    /* 10 channels: (pipe << 1) | is_second_buffer */

    /* master port 1 (dma) */
    for (group = 0; group < QOS_DSS0_DMA_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_DSS0_DMA_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_DSS0_DMA_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_DSS0_DMA_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_DSS0_DMA_CBASS_MAP(channel), (QOS_DSS0_DMA_ATYPE << 28) | (QOS_DSS0_DMA_PRIORITY << 12) | (QOS_DSS0_DMA_ORDER_ID << 4));
    }

    /* master port 2 (fbdc) */
    for (group = 0; group < QOS_DSS0_FBDC_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_DSS0_FBDC_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_DSS0_FBDC_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_DSS0_FBDC_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_DSS0_FBDC_CBASS_MAP(channel), (QOS_DSS0_FBDC_ATYPE << 28) | (QOS_DSS0_FBDC_PRIORITY << 12) | (QOS_DSS0_FBDC_ORDER_ID << 4));
    }
}

void setup_gpu_qos(void)
{
    unsigned int channel, group;

    /* gpu m0 rd */
    for (group = 0; group < QOS_GPU0_M0_RD_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_GPU0_M0_RD_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_GPU0_M0_RD_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_GPU0_M0_RD_NUM_I_CH; ++channel)
    {
        if(channel == 0)
        {
            CSL_REG32_WR(QOS_GPU0_M0_RD_CBASS_MAP(channel), (QOS_GPU0_M0_RD_ATYPE << 28) | (QOS_GPU0_M0_RD_MMU_PRIORITY << 12) | (QOS_GPU0_M0_RD_ORDER_ID << 4));
        }
        else
        {
            CSL_REG32_WR(QOS_GPU0_M0_RD_CBASS_MAP(channel), (QOS_GPU0_M0_RD_ATYPE << 28) | (QOS_GPU0_M0_RD_PRIORITY << 12) | (QOS_GPU0_M0_RD_ORDER_ID << 4));
        }
    }

    /* gpu m0 wr */
    for (group = 0; group < QOS_GPU0_M0_WR_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_GPU0_M0_WR_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_GPU0_M0_WR_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_GPU0_M0_WR_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_GPU0_M0_WR_CBASS_MAP(channel), (QOS_GPU0_M0_WR_ATYPE << 28) | (QOS_GPU0_M0_WR_PRIORITY << 12) | (QOS_GPU0_M0_WR_ORDER_ID << 4));
    }

    /* gpu m1 rd */
    for (group = 0; group < QOS_GPU0_M1_RD_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_GPU0_M1_RD_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_GPU0_M1_RD_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_GPU0_M1_RD_NUM_I_CH; ++channel)
    {
        if(channel == 0)
        {
            CSL_REG32_WR(QOS_GPU0_M1_RD_CBASS_MAP(channel), (QOS_GPU0_M1_RD_ATYPE << 28) | (QOS_GPU0_M1_RD_MMU_PRIORITY << 12) | (QOS_GPU0_M1_RD_ORDER_ID << 4));
        }
        else
        {
            CSL_REG32_WR(QOS_GPU0_M1_RD_CBASS_MAP(channel), (QOS_GPU0_M1_RD_ATYPE << 28) | (QOS_GPU0_M1_RD_PRIORITY << 12) | (QOS_GPU0_M1_RD_ORDER_ID << 4));
        }
    }

    /* gpu m1 wr */
    for (group = 0; group < QOS_GPU0_M1_WR_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_GPU0_M1_WR_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_GPU0_M1_WR_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_GPU0_M1_WR_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_GPU0_M1_WR_CBASS_MAP(channel), (QOS_GPU0_M1_WR_ATYPE << 28) | (QOS_GPU0_M1_WR_PRIORITY << 12) | (QOS_GPU0_M1_WR_ORDER_ID << 4));
    }
}

void setup_encoder_qos(void)
{
    unsigned int channel, group;

    /* encoder rd */
    for (group = 0; group < QOS_ENCODER0_RD_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_ENCODER0_RD_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_ENCODER0_RD_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_ENCODER0_RD_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_ENCODER0_RD_CBASS_MAP(channel), (QOS_ENCODER0_RD_ATYPE << 28) | (QOS_ENCODER0_RD_PRIORITY << 12) | (QOS_ENCODER0_RD_ORDER_ID << 4));
    }

    /* encoder wr */
    for (group = 0; group < QOS_ENCODER0_WR_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_ENCODER0_WR_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_ENCODER0_WR_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_ENCODER0_WR_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_ENCODER0_WR_CBASS_MAP(channel), (QOS_ENCODER0_WR_ATYPE << 28) | (QOS_ENCODER0_WR_PRIORITY << 12) | (QOS_ENCODER0_WR_ORDER_ID << 4));
    }
}

void setup_decoder_qos(void)
{
    unsigned int channel, group;

    /* decoder rd */
    for (group = 0; group < QOS_DECODER0_RD_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_DECODER0_RD_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_DECODER0_RD_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_DECODER0_RD_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_DECODER0_RD_CBASS_MAP(channel), (QOS_DECODER0_RD_ATYPE << 28) | (QOS_DECODER0_RD_PRIORITY << 12) | (QOS_DECODER0_RD_ORDER_ID << 4));
    }

    /* decoder wr */
    for (group = 0; group < QOS_DECODER0_WR_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_DECODER0_WR_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_DECODER0_WR_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_DECODER0_WR_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_DECODER0_WR_CBASS_MAP(channel), (QOS_DECODER0_WR_ATYPE << 28) | (QOS_DECODER0_WR_PRIORITY << 12) | (QOS_DECODER0_WR_ORDER_ID << 4));
    }
}

void setup_c66_qos(void)
{
    unsigned int channel, group;
    uint32_t mapValue;

    /* C66SS0 MDMA: RT order domain 8, epriority 0 (highest). */
    for (group = 0; group < QOS_C66SS0_MDMA_NUM_J_CH; ++group)
    {
        qos_mmr_write(QOS_C66SS0_MDMA_CBASS_GRP_MAP1(group), 0x76543210U);
        qos_mmr_write(QOS_C66SS0_MDMA_CBASS_GRP_MAP2(group), 0xfedcba98U);
    }

    mapValue = (QOS_C66SS0_MDMA_ATYPE << 28) |
               (QOS_C66SS0_MDMA_PRIORITY << 12) |
               (QOS_C66SS0_MDMA_ORDER_ID << 4);
    for (channel = 0; channel < QOS_C66SS0_MDMA_NUM_I_CH; ++channel)
    {
        qos_mmr_write(QOS_C66SS0_MDMA_CBASS_MAP(channel), mapValue);
    }

    /* C66SS1 MDMA: RT order domain 9, epriority 0 (highest). */
    for (group = 0; group < QOS_C66SS1_MDMA_NUM_J_CH; ++group)
    {
        qos_mmr_write(QOS_C66SS1_MDMA_CBASS_GRP_MAP1(group), 0x76543210U);
        qos_mmr_write(QOS_C66SS1_MDMA_CBASS_GRP_MAP2(group), 0xfedcba98U);
    }

    mapValue = (QOS_C66SS1_MDMA_ATYPE << 28) |
               (QOS_C66SS1_MDMA_PRIORITY << 12) |
               (QOS_C66SS1_MDMA_ORDER_ID << 4);
    for (channel = 0; channel < QOS_C66SS1_MDMA_NUM_I_CH; ++channel)
    {
        qos_mmr_write(QOS_C66SS1_MDMA_CBASS_MAP(channel), mapValue);
    }

    printf_("C66 QoS: SS0 MAP=0x%08x, SS1 MAP=0x%08x\r\n",
            CSL_REG32_RD(QOS_C66SS0_MDMA_CBASS_MAP(0)),
            CSL_REG32_RD(QOS_C66SS1_MDMA_CBASS_MAP(0)));
}

void setup_main_r5f_qos(void)
{
    unsigned int channel, group;

    /* R5FSS0 core0 - read */
    for (group = 0; group < QOS_R5FSS0_CORE0_MEM_RD_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE0_MEM_RD_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_R5FSS0_CORE0_MEM_RD_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_R5FSS0_CORE0_MEM_RD_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE0_MEM_RD_CBASS_MAP(channel), (QOS_R5FSS0_CORE0_MEM_RD_ATYPE << 28) | (QOS_R5FSS0_CORE0_MEM_RD_PRIORITY << 12) | (QOS_R5FSS0_CORE0_MEM_RD_ORDER_ID << 4));
    }

    /* R5FSS0 core0 - write */
    for (group = 0; group < QOS_R5FSS0_CORE0_MEM_WR_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE0_MEM_WR_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_R5FSS0_CORE0_MEM_WR_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_R5FSS0_CORE0_MEM_WR_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE0_MEM_WR_CBASS_MAP(channel), (QOS_R5FSS0_CORE0_MEM_WR_ATYPE << 28) | (QOS_R5FSS0_CORE0_MEM_WR_PRIORITY << 12) | (QOS_R5FSS0_CORE0_MEM_RD_ORDER_ID << 4));
    }

    /* R5FSS0 core1 - read */
    for (group = 0; group < QOS_R5FSS0_CORE1_MEM_RD_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE1_MEM_RD_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_R5FSS0_CORE1_MEM_RD_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_R5FSS0_CORE1_MEM_RD_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE1_MEM_RD_CBASS_MAP(channel), (QOS_R5FSS0_CORE1_MEM_RD_ATYPE << 28) | (QOS_R5FSS0_CORE1_MEM_RD_PRIORITY << 12) | (QOS_R5FSS0_CORE0_MEM_RD_ORDER_ID << 4));
    }

    /* R5FSS0 core1 - write */
    for (group = 0; group < QOS_R5FSS0_CORE1_MEM_WR_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE1_MEM_WR_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_R5FSS0_CORE1_MEM_WR_CBASS_GRP_MAP2(group), 0xfedcba98);
    }

    for (channel = 0; channel < QOS_R5FSS0_CORE1_MEM_WR_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_R5FSS0_CORE1_MEM_WR_CBASS_MAP(channel), (QOS_R5FSS0_CORE1_MEM_WR_ATYPE << 28) | (QOS_R5FSS0_CORE1_MEM_WR_PRIORITY << 12) | (QOS_R5FSS0_CORE1_MEM_RD_ORDER_ID << 4));
    }
}

/* DDRSS 鍩哄潃锛圝721E 鍗� DDR 瀹炰緥锛屽熀鍧� 0x02980000锛�*/
#define DDRSS0_V2A_BASE                    (0x02980000U)

#define DDRSS0_V2A_CTL_REG                (DDRSS0_V2A_BASE + 0x20U)
#define DDRSS0_V2A_R1_MAT_REG             (DDRSS0_V2A_BASE + 0x24U)
#define DDRSS0_V2A_R2_MAT_REG             (DDRSS0_V2A_BASE + 0x28U)
#define DDRSS0_V2A_R3_MAT_REG             (DDRSS0_V2A_BASE + 0x2CU)
#define DDRSS0_V2A_LPT_DEF_PRI_MAP_REG    (DDRSS0_V2A_BASE + 0x30U)
#define DDRSS0_V2A_LPT_R1_PRI_MAP_REG     (DDRSS0_V2A_BASE + 0x34U)
#define DDRSS0_V2A_LPT_R2_PRI_MAP_REG     (DDRSS0_V2A_BASE + 0x38U)
#define DDRSS0_V2A_LPT_R3_PRI_MAP_REG     (DDRSS0_V2A_BASE + 0x3CU)
#define DDRSS0_V2A_HPT_DEF_PRI_MAP_REG    (DDRSS0_V2A_BASE + 0x4CU)
#define DDRSS0_V2A_HPT_R1_PRI_MAP_REG     (DDRSS0_V2A_BASE + 0x50U)
#define DDRSS0_V2A_HPT_R2_PRI_MAP_REG     (DDRSS0_V2A_BASE + 0x54U)
#define DDRSS0_V2A_HPT_R3_PRI_MAP_REG     (DDRSS0_V2A_BASE + 0x58U)

/* Range 鍖归厤鍊硷細涓嶅惎鐢� Range-based 鏄犲皠锛屽叏閮ㄨ蛋 DEF 鏄犲皠 */
#define DDRSS_V2A_R1_A72                  (0x90008004U) /* 0x000-001, 0x004 */
#define DDRSS_V2A_R2_C7X_DRU0             (0x800C8068U) /* 0x00c, 0x068 */
#define DDRSS_V2A_R3_DISABLE              (0x00000000U)

/* 浼樺厛绾ф槧灏勫�硷細
 * 姣忎釜 VBUSM priority (0-7) 瀵瑰簲 4-bit DDR priority
 * 0x77777777 = 姣忎釜 VBUSM pri 閮芥槧灏勫埌 DDR pri 7锛堟墎骞冲寲锛岃 VBUSM 鍘熷浼樺厛绾х敓鏁堬級
 *
 * 浣嗘垜浠兂瑕佺殑鏄細璁� EPRIORITY=0 鐨� C7x 娴侀噺鑾峰緱 DDR 鏈�楂樹紭鍏堢骇
 * 鑰� A72 鑳屾櫙娴侀噺铏界劧 OrderID=8锛屼絾鍏� EPRIORITY 涔熷簲璇ヤ綆浜� C7x
 *
 * 鏇村ソ鐨勬槧灏勶細VBUSM pri 0鈫扗DR pri 0(鏈�楂�), pri 7鈫扗DR pri 7(鏈�浣�)
 * 鍗�: pri_map_val = 0x01234567
 *   bit[3:0]   = VBUSM pri 0 鈫� DDR pri 0 (鏈�楂�)
 *   bit[7:4]   = VBUSM pri 1 鈫� DDR pri 1
 *   bit[11:8]  = VBUSM pri 2 鈫� DDR pri 2
 *   bit[15:12] = VBUSM pri 3 鈫� DDR pri 3
 *   bit[19:16] = VBUSM pri 4 鈫� DDR pri 4
 *   bit[23:20] = VBUSM pri 5 鈫� DDR pri 5
 *   bit[27:24] = VBUSM pri 6 鈫� DDR pri 6
 *   bit[31:28] = VBUSM pri 7 鈫� DDR pri 7 (鏈�浣�)
 */
#define DDRSS_V2A_PRI_MAP_LINEAR          (0x01234567U)

/* HPT 榛樿鏄犲皠锛欻PT 鍐呯殑浜嬪姟锛岀嚎鎬ф槧灏� VBUSM priority */
/* LPT 榛樿鏄犲皠锛歀PT 鍐呯殑浜嬪姟锛屽帇浣庢墍鏈変紭鍏堢骇鍒� DDR pri 7锛堟渶浣庯級*/

static void setup_ddrss_cos(void)
{
    /*
     * Direct C7x transactions are OrderID 0 in CPTracer.  Do not demote the
     * complete LPT class; select A72 and C7x/DRU0 by Route ID instead.
     */
    /* 1. 绂佺敤 Range-based 鏄犲皠锛屽叏閮ㄨ蛋 DEF 鏄犲皠 */
    CSL_REG32_WR(DDRSS0_V2A_R1_MAT_REG, DDRSS_V2A_R1_A72);
    CSL_REG32_WR(DDRSS0_V2A_R2_MAT_REG, DDRSS_V2A_R2_C7X_DRU0);
    CSL_REG32_WR(DDRSS0_V2A_R3_MAT_REG, DDRSS_V2A_R3_DISABLE);

    /* 2. LPT 榛樿浼樺厛绾ф槧灏勶細鍘嬩綆鍒� DDR pri 7锛堟渶浣庯級
     * 鐢变簬 NAVSS NB 宸茬粡鎶� OrderID 0-7 鏄犲皠鍒� thread 0 (LPT)锛�
     * 杩欎簺娴侀噺锛圕66x, GPU浣庝紭鍏堢骇閫氶亾绛夛級鍦� DDR 绔幏寰楁渶浣庝紭鍏堢骇 */
    CSL_REG32_WR(DDRSS0_V2A_LPT_DEF_PRI_MAP_REG, DDRSS_V2A_PRI_MAP_LINEAR);
    CSL_REG32_WR(DDRSS0_V2A_LPT_R1_PRI_MAP_REG, 0x77777777U);
    CSL_REG32_WR(DDRSS0_V2A_LPT_R2_PRI_MAP_REG, 0x00000000U);
    CSL_REG32_WR(DDRSS0_V2A_LPT_R3_PRI_MAP_REG, DDRSS_V2A_PRI_MAP_LINEAR);

    /* 3. HPT 榛樿浼樺厛绾ф槧灏勶細绾挎�ф槧灏勶紝淇濈暀 VBUSM EPRIORITY 淇℃伅
     * NAVSS NB 鎶� OrderID 8-15 鏄犲皠鍒� thread 2 (HPT)锛�
     * C7x (OrderID=8, EPRIORITY=0) 灏嗚幏寰� DDR pri 0锛堟渶楂橈級
     * A72 鑳屾櫙娴侀噺锛堝鏋滀篃鐢� OrderID=8锛夊悓鏍疯繘鍏� HPT锛�
     * 浣嗙敱浜庡叾 EPRIORITY 鍙兘涓嶄负 0锛屼細鍦� HPT 鍐呴儴鎺掑埌 C7x 鍚庨潰 */
    CSL_REG32_WR(DDRSS0_V2A_HPT_DEF_PRI_MAP_REG, DDRSS_V2A_PRI_MAP_LINEAR);
    CSL_REG32_WR(DDRSS0_V2A_HPT_R1_PRI_MAP_REG, 0x77777777U);
    CSL_REG32_WR(DDRSS0_V2A_HPT_R2_PRI_MAP_REG, 0x00000000U);
    CSL_REG32_WR(DDRSS0_V2A_HPT_R3_PRI_MAP_REG, DDRSS_V2A_PRI_MAP_LINEAR);

    // /* 4. 鍚敤 DDR 鎺у埗鍣ㄧ殑浼樺厛绾ц皟搴︼紙鍙傝�� TI E2E ZHDA076锛�*/
    // /* DDRSS_CTL_276[0] (PRIORITY_EN) = 1 */
    // volatile uint32_t *ddrss_ctl = (volatile uint32_t *)0x4F200450;  /* DDRSS_CTL_276 鍋忕Щ锛岄渶鏍稿 */
    // *ddrss_ctl |= (1U << 0);

    // /* DDRSS_CTL_377[24] (AXI0_FIXED_PORT_PRIORITY_ENABLE) = 0
    //  * 绂佺敤鍥哄畾绔彛浼樺厛绾э紝璁� VBUSM 浼樺厛绾х敓鏁� */
    // volatile uint32_t *ddrss_ctl_377 = (volatile uint32_t *)0x4F2005E4;  /* 闇�鏍稿 */
    // *ddrss_ctl_377 &= ~(1U << 24);


}


void setup_com_qos(void)
{
    unsigned int channel, group;

    /* RD 閫氶亾锛氬亸鍚� C7x 瀹炴椂鎬� */
    for (group = 0; group < QOS_COMPUTE_CLUSTER0_RD_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_COMPUTE_CLUSTER0_RD_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_COMPUTE_CLUSTER0_RD_CBASS_GRP_MAP2(group), 0xfedcba98);
    }
    for (channel = 0; channel < QOS_COMPUTE_CLUSTER0_RD_NUM_I_CH; ++channel)
    {
        /* OrderID=8 (RT闃熷垪), EPRIORITY=0 (鏈�楂�), QoS=7, ATYPE=0 */
        CSL_REG32_WR(QOS_COMPUTE_CLUSTER0_RD_CBASS_MAP(channel),
            (0U << 28) | (0U << 12) | (8U << 4) | (7U << 0));
    }

    /* WR 閫氶亾锛氬悓涓� */
    for (group = 0; group < QOS_COMPUTE_CLUSTER0_WR_NUM_J_CH; ++group)
    {
        CSL_REG32_WR(QOS_COMPUTE_CLUSTER0_WR_CBASS_GRP_MAP1(group), 0x76543210);
        CSL_REG32_WR(QOS_COMPUTE_CLUSTER0_WR_CBASS_GRP_MAP2(group), 0xfedcba98);
    }
    for (channel = 0; channel < QOS_COMPUTE_CLUSTER0_WR_NUM_I_CH; ++channel)
    {
        CSL_REG32_WR(QOS_COMPUTE_CLUSTER0_WR_CBASS_MAP(channel),
            (0U << 28) | (0U << 12) | (8U << 4) | (7U << 0));
    }
}

/* MSMC 鍩哄潃 */
#define MSMC0_CFG_BASE                (0x6E000000U)

/* MSMC 瀵勫瓨鍣ㄥ亸绉伙紙鍩轰簬 SPRUIL1 TRM Table 3-2锛� */
#define MSMC_CACHE_CTRL               (MSMC0_CFG_BASE + 0x1000U)
#define MSMC_RT_WAY_SELECT            (MSMC0_CFG_BASE + 0x1010U)
#define MSMC_NRT_WAY_SELECT           (MSMC0_CFG_BASE + 0x1018U)
#define MSMC_COHCTRL                  (MSMC0_CFG_BASE + 0x2048U)

/* 楗ラタ杈圭晫瀵勫瓨鍣� - 鐩稿共绔彛 */
#define MSMC_SBNDCOH0                 (MSMC0_CFG_BASE + 0x6000U)  /* Coh Port 0 - A72 cluster 0 */
#define MSMC_SBNDCOH1                 (MSMC0_CFG_BASE + 0x6008U)  /* Coh Port 1 - 鏈娇鐢� */
#define MSMC_SBNDCOH2                 (MSMC0_CFG_BASE + 0x6010U)  /* Coh Port 2 - 鏈娇鐢� */
#define MSMC_SBNDCOH3                 (MSMC0_CFG_BASE + 0x6018U)  /* Coh Port 3 - 鏈娇鐢� */
#define MSMC_SBNDCOH4                 (MSMC0_CFG_BASE + 0x6020U)  /* Coh Port 4 - C7x */
#define MSMC_SBNDCOH5                 (MSMC0_CFG_BASE + 0x6028U)  /* Coh Port 5 - 鏈娇鐢� */
#define MSMC_SBNDCOH6                 (MSMC0_CFG_BASE + 0x6030U)  /* Coh Port 6 - 鏈娇鐢� */
#define MSMC_SBNDCOH7                 (MSMC0_CFG_BASE + 0x6038U)  /* Coh Port 7 - 鏈娇鐢� */
#define MSMC_SBNDCOH8                 (MSMC0_CFG_BASE + 0x6040U)  /* Coh Port 8 - 鏈娇鐢� */
#define MSMC_SBNDCOH9                 (MSMC0_CFG_BASE + 0x6048U)  /* Coh Port 9 - 鏈娇鐢� */
#define MSMC_SBNDCOH10                (MSMC0_CFG_BASE + 0x6050U)  /* Coh Port 10 - 鏈娇鐢� */
#define MSMC_SBNDCOH11                (MSMC0_CFG_BASE + 0x6058U)  /* Coh Port 11 - 鍦ㄧ敤 */
#define MSMC_SBNDCOH12                (MSMC0_CFG_BASE + 0x6060U)  /* Coh Port 12 - DRU */

/* 楗ラタ杈圭晫瀵勫瓨鍣� - DRU 鍜� Read Response */
#define MSMC_SBNDDRU                  (MSMC0_CFG_BASE + 0x6100U)  /* DRU 楗ラタ杈圭晫 */
#define MSMC_SBNDRESP                 (MSMC0_CFG_BASE + 0x6200U)  /* Read Response 楗ラタ杈圭晫 */

/* 闃查タ姝婚槇鍊�
 *
 * 鏁板�煎惈涔夛細杩炵画浜嬪姟璁℃暟鐨勪笂闄愩�傝揪鍒拌鍊煎悗 MSMC 浠茶鍣ㄥ己鍒跺垏鎹紝
 * 璁╃瓑寰呮渶涔呯殑绔彛鍏堣蛋锛岄槻姝㈤暱娴侀タ姝荤煭娴併��
 *
 * 鍊艰秺灏� 鈫� 浠茶瓒婇绻� 鈫� 瀹炴椂娴侀噺寤惰繜瓒婄ǔ瀹氾紝浣嗗悶鍚愰噺鐣ラ檷
 * 鍊艰秺澶� 鈫� 鍚炲悙閲忚秺楂� 鈫� 浣嗗疄鏃舵祦閲忓彲鑳借闀挎祦闃诲鏇翠箙
 *
 * 閽堝 C7x 瀹炴椂鎬т紭鍖栵細
 * - C7x (Coh Port 4)锛氳鏈�灏忓�� 0x01锛岀‘淇� C7x 鍑犱箮涓嶈楗挎
 * - DRU (Coh Port 12)锛氳涓瓑鍊� 0x08锛屼繚璇� DRU 楂樺悶鍚愪笖涓嶈楗挎
 * - A72 (Coh Port 0)锛氳杈冨ぇ鍊� 0x40锛屽厑璁� A72 闀跨獊鍙戯紝浣嗗湪 C7x 绛夊緟瓒呮椂鍚庡繀椤昏鍑�
 * - SBNDRESP锛氳杈冨皬鍊� 0x10锛岄槻姝㈣鍝嶅簲閫氶亾琚樆濉�
 *
 * 杩欎簺鍊兼槸璧风偣锛岄渶鏍规嵁瀹為檯鍦烘櫙寰皟銆�
 */
#define MSMC_SBNDCOH_C7X_VAL          (0x01U)   /* C7x (Coh Port 4) - 鏈�婵�杩� */
#define MSMC_SBNDCOH_DRU_VAL          (0x08U)   /* DRU (Coh Port 12) - 涓瓑 */
#define MSMC_SBNDCOH_A72_VAL          (0x80U)   /* A72 (Coh Port 0) - 杈冨鏉� */
#define MSMC_SBNDCOH_OTHER_VAL        (0x20U)   /* 鍏朵粬鍦ㄧ敤绔彛 - 涓瓑 */
#define MSMC_SBNDDRU_VAL              (0x08U)   /* DRU 楗ラタ杈圭晫 */
#define MSMC_SBNDRESP_VAL             (0x10U)   /* Read Response 楗ラタ杈圭晫 */

static void set_msmc_starvation_bound(uint32_t regAddr, uint32_t value)
{
    /*
     * Each SBND register is 64 bits.  Program external/on-chip bounds for
     * both NRT (low word) and RT (high word); a 32-bit write of just value
     * changes only SBNDM_NRT and leaves the other three fields untouched.
     */
    uint32_t word = ((value & 0xFFU) << 16) | (value & 0xFFU);

    CSL_REG32_WR(regAddr, word);
    CSL_REG32_WR(regAddr + 4U, word);
}

static void setup_msmc_qos(void)
{
    /* 1. C7x (Coh Port 4) - 鏈�婵�杩涚殑闃查タ姝�
     * C7x 鏄疄鏃� DSP锛屽繀椤昏幏寰楁渶浣庣殑寤惰繜鎶栧姩 */
    set_msmc_starvation_bound(MSMC_SBNDCOH4, MSMC_SBNDCOH_C7X_VAL);

    /* 2. DRU (Coh Port 12) - 涓瓑闃查タ姝�
     * DRU 鎵挎媴 C7x 鐨勯珮甯﹀ DMA锛岄渶瑕佽緝楂樺悶鍚愶紝浣嗕粛闇�闃查タ姝� */
    set_msmc_starvation_bound(MSMC_SBNDCOH12, MSMC_SBNDCOH_DRU_VAL);
    set_msmc_starvation_bound(MSMC_SBNDDRU, MSMC_SBNDDRU_VAL);

    /* 3. A72 (Coh Port 0) - 杈冨鏉�
     * A72 杩愯 Linux锛屽厑璁歌緝闀跨獊鍙戜互鑾峰緱楂樺悶鍚愰噺
     * 浣嗗綋 C7x 绛夊緟瓒呰繃 0x40 涓懆鏈熷悗锛孧SMC 浠茶鍣ㄤ細寮哄埗鍒囨崲 */
    set_msmc_starvation_bound(MSMC_SBNDCOH0, MSMC_SBNDCOH_A72_VAL);

    /* 4. 鍏朵粬鍦ㄧ敤鐩稿共绔彛 - 涓瓑闃查タ姝�
     * Coh Port 11 绛� */
    set_msmc_starvation_bound(MSMC_SBNDCOH11, MSMC_SBNDCOH_OTHER_VAL);

    /* 5. Read Response 楗ラタ杈圭晫
     * 闃叉璇绘暟鎹繑鍥為�氶亾琚暱娴侀樆濉� */
    set_msmc_starvation_bound(MSMC_SBNDRESP, MSMC_SBNDRESP_VAL);

    /* 6. 鍙�夛細RT/NRT Way Select
     *
     * 濡傛灉 MSMC 澶ч儴鍒嗙敤浣� L3 Cache锛屼笉寤鸿鍒掑垎 RT/NRT way锛�
     * 鍚﹀垯浼氬奖鍝� NRT 娴侀噺鐨勭紦瀛樺懡涓巼銆�
     *
     * 浠呭湪浠ヤ笅鍦烘櫙鍚敤锛�
     * - MSMC 涓昏閰嶇疆涓� SRAM锛堥�氳繃 SYSFW boardcfg msmc_cache_size=0x0锛�
     * - 涓旈渶瑕佷负 C7x 瀹炴椂鏁版嵁棰勭暀涓撶敤 SRAM bank
     *
     * 绀轰緥锛�8MB MSMC = 16 way 脳 512KB
     * RT_WAY_SELECT = 0x00FF 鈫� way 0-7 缁� RT 娴侀噺
     * NRT_WAY_SELECT = 0xFF00 鈫� way 8-15 缁� NRT 娴侀噺
     */
    /* CSL_REG32_WR(MSMC_RT_WAY_SELECT, 0x00FFU); */
    /* CSL_REG32_WR(MSMC_NRT_WAY_SELECT, 0xFF00U); */
}


/* This function is to be called from other apps (e.g., mcusw boot app) to set QoS settings */
void SBL_SetQoS(void)
{
    printf_("SBL_SetQoS: early QoS start.\r\n");
    setup_navss_nb();
    setup_c66_qos();
    /* Workaround to unblock PDK-8359 .
     * setup_main_r5f_qos() results in crashing the UDMA DRU examples on
     * mcu2_0(with SBL uart boot mode) during CSL_REG64_WR(&pRegs->DRUQueues.CFG[queueId], regVal);
     * Hence commenting out the following. */
    //setup_main_r5f_qos();
    //setup_vpac_qos();
    //setup_dmpac_qos();
    //setup_dss_qos();
    //setup_gpu_qos();
    //setup_encoder_qos();
    /* 0x45D86000/0x45D86400 are GIC0 RD/WR QoS, not direct C7x/A72. */
//    setup_com_qos();
    /* DDR CoS is applied after DDR register init by Board_DDRPriorityInit(). */
    //setup_ddrss_cos();
    /*
     * Do not touch the MSMC configuration space here.  SBL_SocLateInit()
     * runs before the main-domain PLL/clock and DDR initialization in
     * sbl_main.c.  Accessing 0x6e00xxxx at this point can stall MCU R5F.
     * SBL_SetMSMCQoS() is called after Board_init(BOARD_INIT_DDR) succeeds.
     */

    printf_("SBL_SetQoS: early QoS end.\r\n");
}

void SBL_SetMSMCQoS(void)
{
    printf_("SBL_SetMSMCQoS: start.\r\n");
    setup_msmc_qos();
    printf_("SBL_SetMSMCQoS: end.\r\n");
}

