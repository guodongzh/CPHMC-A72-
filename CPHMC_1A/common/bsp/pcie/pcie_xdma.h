/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_xdma.h
 *@author     LiuRui
 *@date       2024.07.12
 *@brief      xilinx pcie xdma api
 *@par        History
 *Date        Version   Author     Description
 *2024.07.12  1.0       LiuRui    first version
 ******************************************************************************/

#ifndef PCIE_XDMA_H
#define PCIE_XDMA_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <ti/csl/cslr.h>

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define XDMA_CHANNEL_NUM_MAX             (1)
#define XDMA_BAR_NUM                     (6)

#define MAX_PKT_LEN                      (128)
#define H2C_DMA_LEN                      (MAX_PKT_LEN * 4)
#define C2H_DMA_LEN                      (MAX_PKT_LEN * 4)

#define AXI_ADDRESS                      (0x0)
#define XDMA_TO_DEVICE                   0
#define XDMA_FROM_DEVICE                 1


#define IRQ_BLOCK_ID 0x1fc20000UL
#define CONFIG_BLOCK_ID 0x1fc30000UL


/*
 * interrupts per engine, rad2_vul.sv:237
 * .REG_IRQ_OUT	(reg_irq_from_ch[(channel*2) +: 2]),
 */
#define XDMA_ENG_IRQ_NUM                 (1)
#define XDMA_MAX_ADJ_BLOCK_SIZE          0x40
#define XDMA_PAGE_SIZE                   0x1000
#define RX_STATUS_EOP                    (1)

/* bits of the SGDMA descriptor control field */
#define XDMA_DESC_STOPPED                (1UL << 0)
#define XDMA_DESC_COMPLETED              (1UL << 1)
#define XDMA_DESC_EOP                    (1UL << 4)
#define DESC_MAGIC                       0xAD4B0000UL

/* bits of the SG DMA control register */
#define XDMA_CTRL_RUN_STOP               (1UL << 0)
#define XDMA_CTRL_IE_DESC_STOPPED        (1UL << 1)
#define XDMA_CTRL_IE_DESC_COMPLETED      (1UL << 2)
#define XDMA_CTRL_IE_DESC_ALIGN_MISMATCH (1UL << 3)
#define XDMA_CTRL_IE_MAGIC_STOPPED       (1UL << 4)
#define XDMA_CTRL_IE_IDLE_STOPPED        (1UL << 6)
#define XDMA_CTRL_IE_READ_ERROR          (0x1FUL << 9)
#define XDMA_CTRL_IE_DESC_ERROR          (0x1FUL << 19)
#define XDMA_CTRL_NON_INCR_ADDR          (1UL << 25)
#define XDMA_CTRL_POLL_MODE_WB           (1UL << 26)
#define XDMA_CTRL_STM_MODE_WB            (1UL << 27)

/* bits of the SG DMA status register */
#define XDMA_STAT_BUSY                   (1UL << 0)
#define XDMA_STAT_DESC_STOPPED           (1UL << 1)
#define XDMA_STAT_DESC_COMPLETED         (1UL << 2)
#define XDMA_STAT_ALIGN_MISMATCH         (1UL << 3)
#define XDMA_STAT_MAGIC_STOPPED          (1UL << 4)
#define XDMA_STAT_INVALID_LEN            (1UL << 5)
#define XDMA_STAT_IDLE_STOPPED           (1UL << 6)

#define XDMA_STAT_COMMON_ERR_MASK                                                                  \
    (XDMA_STAT_ALIGN_MISMATCH | XDMA_STAT_MAGIC_STOPPED | XDMA_STAT_INVALID_LEN)

/* desc_error, C2H & H2C */
#define XDMA_STAT_DESC_UNSUPP_REQ   (1UL << 19)
#define XDMA_STAT_DESC_COMPL_ABORT  (1UL << 20)
#define XDMA_STAT_DESC_PARITY_ERR   (1UL << 21)
#define XDMA_STAT_DESC_HEADER_EP    (1UL << 22)
#define XDMA_STAT_DESC_UNEXP_COMPL  (1UL << 23)


// clang-format off
#define XDMA_STAT_DESC_ERR_MASK    \
    (                              \
    XDMA_STAT_DESC_UNSUPP_REQ   |  \
    XDMA_STAT_DESC_COMPL_ABORT  |  \
    XDMA_STAT_DESC_PARITY_ERR   |  \
    XDMA_STAT_DESC_HEADER_EP    |  \
    XDMA_STAT_DESC_UNEXP_COMPL     \
    )
// clang-format on

/* read error: H2C */
#define XDMA_STAT_H2C_R_UNSUPP_REQ  (1UL << 9)
#define XDMA_STAT_H2C_R_COMPL_ABORT (1UL << 10)
#define XDMA_STAT_H2C_R_PARITY_ERR  (1UL << 11)
#define XDMA_STAT_H2C_R_HEADER_EP   (1UL << 12)
#define XDMA_STAT_H2C_R_UNEXP_COMPL (1UL << 13)

#define XDMA_STAT_H2C_R_ERR_MASK                                                                   \
    (XDMA_STAT_H2C_R_UNSUPP_REQ | XDMA_STAT_H2C_R_COMPL_ABORT | XDMA_STAT_H2C_R_PARITY_ERR |       \
     XDMA_STAT_H2C_R_HEADER_EP | XDMA_STAT_H2C_R_UNEXP_COMPL)

/* write error, H2C only */
#define XDMA_STAT_H2C_W_DECODE_ERR (1UL << 14)
#define XDMA_STAT_H2C_W_SLAVE_ERR  (1UL << 15)

#define XDMA_STAT_H2C_W_ERR_MASK   (XDMA_STAT_H2C_W_DECODE_ERR | XDMA_STAT_H2C_W_SLAVE_ERR)

/* read error: C2H */
#define XDMA_STAT_C2H_R_DECODE_ERR (1UL << 9)
#define XDMA_STAT_C2H_R_SLAVE_ERR  (1UL << 10)

#define XDMA_STAT_C2H_R_ERR_MASK   (XDMA_STAT_C2H_R_DECODE_ERR | XDMA_STAT_C2H_R_SLAVE_ERR)

/* all combined */
#define XDMA_STAT_H2C_ERR_MASK                                                                     \
    (XDMA_STAT_COMMON_ERR_MASK | XDMA_STAT_DESC_ERR_MASK | XDMA_STAT_H2C_R_ERR_MASK |              \
     XDMA_STAT_H2C_W_ERR_MASK)

#define XDMA_STAT_C2H_ERR_MASK                                                                     \
    (XDMA_STAT_COMMON_ERR_MASK | XDMA_STAT_DESC_ERR_MASK | XDMA_STAT_C2H_R_ERR_MASK)

/* obtain the 32 most significant (high) bits of a 32-bit or 64-bit address */
#define PCI_DMA_H(addr)           ((addr >> 16) >> 16)
/* obtain the 32 least significant (low) bits of a 32-bit or 64-bit address */
#define PCI_DMA_L(addr)           (addr & 0xffffffffUL)

/* Target internal components on XDMA control BAR
 * offset from bar
 * */
#define XDMA_OFS_INT_CTRL         (0x2000UL)
#define XDMA_OFS_CONFIG           (0x3000UL)

#define BLOCK_ID_MASK             0xFFF00000
#define BLOCK_ID_HEAD             0x1FC00000

#define H2C_CHANNEL_OFFSET        0x1000
#define SGDMA_OFFSET_FROM_CHANNEL 0x4000
#define CHANNEL_SPACING           0x100
#define TARGET_SPACING            0x1000
#define BYPASS_MODE_SPACING       0x0100

#define write_register(v, mem)    CSL_REG32_WR(mem, v)
#define read_register(mem)        CSL_REG32_RD(mem)

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                              */
/*---------------------------------------------------------------------------*/
struct xdma_desc
{
    uint32_t control;
    uint32_t bytes;       /* transfer length in bytes */
    uint32_t src_addr_lo; /* source address (low 32-bit) */
    uint32_t src_addr_hi; /* source address (high 32-bit) */
    uint32_t dst_addr_lo; /* destination address (low 32-bit) */
    uint32_t dst_addr_hi; /* destination address (high 32-bit) */
    /*
     * next descriptor in the single-linked list of descriptors;
     * this is the PCIe (bus) address of the next descriptor in the
     * root complex memory
     */
    uint32_t next_lo; /* next desc address (low 32-bit) */
    uint32_t next_hi; /* next desc address (high 32-bit) */
} __attribute__((packed));

/**
 * SG DMA Controller status and control registers
 *
 * These registers make the control interface for DMA transfers.
 *
 * It sits in End Point (FPGA) memory BAR[0] for 32-bit or BAR[0:1] for 64-bit.
 * It references the first descriptor which exists in Root Complex (PC) memory.
 *
 * @note The registers must be accessed using 32-bit (PCI DWORD) read/writes,
 * and their values are in little-endian byte ordering.
 */
struct engine_regs
{
    uint32_t identifier;
    uint32_t control;
    uint32_t control_w1s;
    uint32_t control_w1c;
    uint32_t reserved_1[12]; /* padding */

    uint32_t status;
    uint32_t status_rc;
    uint32_t completed_desc_count;
    uint32_t alignments;
    uint32_t reserved_2[14]; /* padding */

    uint32_t poll_mode_wb_lo;
    uint32_t poll_mode_wb_hi;
    uint32_t interrupt_enable_mask;
    uint32_t interrupt_enable_mask_w1s;
    uint32_t interrupt_enable_mask_w1c;
    uint32_t reserved_3[9]; /* padding */

    uint32_t perf_ctrl;
    uint32_t perf_cyc_lo;
    uint32_t perf_cyc_hi;
    uint32_t perf_dat_lo;
    uint32_t perf_dat_hi;
    uint32_t perf_pnd_lo;
    uint32_t perf_pnd_hi;
} __attribute__((packed));

struct interrupt_regs
{
    uint32_t identifier;
    uint32_t user_int_enable;
    uint32_t user_int_enable_w1s;
    uint32_t user_int_enable_w1c;
    uint32_t channel_int_enable;
    uint32_t channel_int_enable_w1s;
    uint32_t channel_int_enable_w1c;
    uint32_t reserved_1[9]; /* padding */
    uint32_t user_int_request;
    uint32_t channel_int_request;
    uint32_t user_int_pending;
    uint32_t channel_int_pending;
    uint32_t reserved_2[12]; /* padding */
    uint32_t user_msi_vector[8];
    uint32_t channel_msi_vector[8];
} __attribute__((packed));

struct config_regs
{
    uint32_t identifier;
    uint32_t bdf;
    uint32_t wr_mps;
    uint32_t rd_mps;
    uint32_t sys_id;
    uint32_t msi_enable;
} __attribute__((packed));

struct engine_sgdma_regs
{
    uint32_t identifier;
    uint32_t reserved_1[31]; /* padding */

    /* bus address to first descriptor in Root Complex Memory */
    uint32_t first_desc_lo;
    uint32_t first_desc_hi;
    /* number of adjacent descriptors at first_desc */
    uint32_t first_desc_adjacent;
    uint32_t credits;
} __attribute__((packed));


struct sgdma_common_regs
{
    uint32_t padding[8];
    uint32_t credit_mode_enable;
    uint32_t credit_mode_enable_w1s;
    uint32_t credit_mode_enable_w1c;
} __packed;


struct xdma_engine
{
    char name[16]; /* name of this engine */
    /* HW register address offsets */
    struct engine_regs *regs;             /* Control reg BAR offset */
    struct engine_sgdma_regs *sgdma_regs; /* SGDAM reg BAR offset */
    uint32_t bypass_offset;               /* Bypass mode BAR offset */

    uint32_t dir;
    uint8_t addr_align;      /* source/dest alignment in bytes */
    uint8_t len_granularity; /* transfer length multiple */
    uint8_t addr_bits;       /* HW datapath address width */
    uint8_t channel : 2;     /* engine indices */
    uint8_t streaming : 1;
    uint8_t device_open : 1;   /* flag if engine node open, ST mode only */
    uint8_t running : 1;       /* flag if the driver started engine */
    uint8_t non_incr_addr : 1; /* flag if non-incremental addressing used */
    uint8_t eop_flush : 1;     /* st c2h only, flush up the data with eop */
    uint8_t filler : 1;

    uint32_t status;      /* last known status of device */
    uint32_t irq_bitmask; /* IRQ bit mask for this engine */
    /* Members associated with polled mode support */
    uint32_t poll_mode_bus; /* bus addr for descriptor writeback */

    void (*do_data)(void);
};

struct xdma_dev
{
    struct pci_device *pdev; /* pci device struct */
    int idx;                 /* dev index */
    const char *mod_name;    /* name of module owning the dev */

    /* PCIe BAR management */
    void *bar[XDMA_BAR_NUM]; /* addresses for mapped BARs */
    int user_bar_idx;        /* BAR index of user logic */
    int config_bar_idx;      /* BAR index of XDMA config logic */
    int bypass_bar_idx;      /* BAR index of XDMA bypass logic */
    int regions_in_use;      /* flag if dev was in use during probe() */

    uint32_t c2h_channel_max;
    uint32_t h2c_channel_max;

    /* Interrupt management */
    int irq_count;   /* interrupt counter */
    int irq_line;    /* flag if irq allocated successfully */
    int msi_enabled; /* flag if msi was enabled for the device */
    int legacy_enabled;

    // struct xdma_user_irq user_irq[16];	/* user IRQ management */
    unsigned int mask_irq_user;

    /* XDMA engine management */
    int engines_num; /* Total engine count */
    uint32_t mask_irq_h2c;
    uint32_t mask_irq_c2h;
    struct xdma_engine engine_h2c[XDMA_CHANNEL_NUM_MAX];
    struct xdma_engine engine_c2h[XDMA_CHANNEL_NUM_MAX];
};

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/

int xdma_desc_h2c_init(int len);
int xdma_desc_c2h_init(int len);
int process_data(int dir);
void pcie_xdma();

#ifdef __cplusplus
}
#endif

#endif  //PCIE_XDMA_H
