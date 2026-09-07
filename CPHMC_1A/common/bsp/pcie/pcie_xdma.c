/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       pcie_xdma.c
 *@author     LiuRui
 *@date       2024.07.12
 *@brief      xilinx pcie xdma api
 *@par        History
 *Date        Version   Author     Description
 *2024.07.12  1.0       LiuRui     first version
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include "pcie_xdma.h"
#include "debug_config.h"
#include "pcie_init.h"

/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */


/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
struct xdma_dev xdma_dev_inst;
ib_buff_t ib_buff_space;

#define H2C_DESC_ADDR (PCIE_IB0_LO_ADDR_RC)
#define C2H_DESC_ADDR (H2C_DESC_ADDR + sizeof(ib_buff_space.xdma_sg_desc_h2c))

#define RX_BUFF_ADDR  (C2H_DESC_ADDR + sizeof(ib_buff_space.xdma_sg_desc_c2h))
#define TX_BUFF_ADDR  (RX_BUFF_ADDR + sizeof(ib_buff_space.rx_buffer))


static int is_config_bar(struct xdma_dev *xdev, int idx)
{
    uint32_t irq_id;
    uint32_t cfg_id;
    int flag;
    uint32_t mask = 0xffff0000; /* Compare only XDMA ID's not Version number */
    struct interrupt_regs *irq_regs = (struct interrupt_regs *)(xdev->bar[idx] + XDMA_OFS_INT_CTRL);
    struct config_regs *cfg_regs = (struct config_regs *)(xdev->bar[idx] + XDMA_OFS_CONFIG);

    irq_id = read_register(&irq_regs->identifier);
    cfg_id = read_register(&cfg_regs->identifier);

    uint32_t wr_mps = read_register(&cfg_regs->wr_mps);
    uint32_t rd_mps = read_register(&cfg_regs->rd_mps);

    Debug_log("wr_mps=%d\n", wr_mps);
    Debug_log("rd_mps=%d\n", rd_mps);

    if (((irq_id & mask) == IRQ_BLOCK_ID) &&
        ((cfg_id & mask) == CONFIG_BLOCK_ID)) {
        Debug_log("BAR %d is the XDMA config BAR\n", idx);
        flag = 1;
    } else {
        Debug_log("BAR %d is NOT the XDMA config BAR: 0x%x, 0x%x.\n",
                 idx, irq_id, cfg_id);
        flag = 0;
    }

    return flag;
}

static uint32_t get_engine_id(struct engine_regs *regs)
{
    uint32_t value;

    if (!regs)
    {
        Debug_log("Invalid engine registers\n");
        return -1;
    }

    value = regs->identifier;
    return (value & 0xffff0000U) >> 16;
}

static uint32_t get_engine_channel_id(struct engine_regs *regs)
{
    uint32_t value;

    if (!regs)
    {
        Debug_log("Invalid engine registers\n");
        return -1;
    }

    value = regs->identifier;

    return (value & 0x00000f00U) >> 8;
}

static void engine_alignments(struct xdma_engine *engine)
{
    uint32_t w, addr_alignment, len_granularity, address_bits;

    w = read_register(&engine->regs->alignments);
    Debug_log("engine %p name %s alignments=0x%08x\n", engine, engine->name, (int)w);

    addr_alignment = (w & 0xffff0000U) >> 16;
    len_granularity = (w & 0x0000ff00U) >> 8;
    address_bits = (w & 0x000000ffU);

    Debug_log("align_bytes = %d byte\n", addr_alignment);
    Debug_log("granularity_bytes = %d byte\n", len_granularity);
    Debug_log("address_bits = %d bit\n", address_bits);

    if (w)
    {
        engine->addr_align = addr_alignment;
        engine->len_granularity = len_granularity;
        engine->addr_bits = address_bits;
    }
    else
    {
        /* Some default values if alignments are unspecified */
        engine->addr_align = 1;
        engine->len_granularity = 1;
        engine->addr_bits = 64;
    }
}

static int engine_init_regs(struct xdma_engine *engine)
{
    uint32_t reg_value;

    write_register(XDMA_CTRL_NON_INCR_ADDR, &engine->regs->control_w1c);
    engine_alignments(engine);

    /* Configure error interrupts by default */
    reg_value = XDMA_CTRL_IE_DESC_ALIGN_MISMATCH;
    reg_value |= XDMA_CTRL_IE_MAGIC_STOPPED;
    reg_value |= XDMA_CTRL_IE_MAGIC_STOPPED;
    reg_value |= XDMA_CTRL_IE_READ_ERROR;
    reg_value |= XDMA_CTRL_IE_DESC_ERROR;

    /* enable the relevant completion interrupts */
    reg_value |= XDMA_CTRL_IE_DESC_STOPPED;
    reg_value |= XDMA_CTRL_IE_DESC_COMPLETED;

    /* Apply engine configurations */
    write_register(reg_value, &engine->regs->interrupt_enable_mask);
    return 0;
}

static int engine_init(struct xdma_engine *engine,
                       struct xdma_dev *xdev,
                       uint32_t offset,
                       int dir,
                       uint32_t channel)
{
    int val;
    int rv;

    engine->channel = channel;

    /* engine interrupt request bit */
    engine->irq_bitmask = (1 << XDMA_ENG_IRQ_NUM) - 1;
    engine->irq_bitmask <<= (xdev->engines_num * XDMA_ENG_IRQ_NUM);
    engine->bypass_offset = xdev->engines_num * BYPASS_MODE_SPACING;

    engine->regs = (xdev->bar[xdev->config_bar_idx] + offset);
    engine->sgdma_regs = xdev->bar[xdev->config_bar_idx] + offset + SGDMA_OFFSET_FROM_CHANNEL;
    Debug_log("found AXI %s engine_regs: 0x%x,engine_sgdma_regs: 0x%x\n",
              dir == 0 ? "H2C" : "C2H",
              engine->regs,
              engine->sgdma_regs);

    val = read_register(&engine->regs->identifier);
    if (val & 0x8000U)
        engine->streaming = 1;

    /* remember SG DMA direction */
    engine->dir = dir;
    snprintf(engine->name,
             sizeof(engine->name),
             "%d-%s%d-%s",
             xdev->idx,
             (dir == 0) ? "H2C" : "C2H",
             channel,
             engine->streaming ? "ST" : "MM");

    if (dir == 0)
        engine->do_data = NULL;
    else
        engine->do_data = NULL;

    if (dir == 0)
        xdev->mask_irq_h2c |= engine->irq_bitmask;
    else
        xdev->mask_irq_c2h |= engine->irq_bitmask;
    xdev->engines_num++;

    engine->running = 0;

    rv = engine_init_regs(engine);

    return rv;
}

static int probe_for_engine(struct xdma_dev *xdev, int dir, uint32_t channel)
{
    int rv;
    uint32_t offset = channel * CHANNEL_SPACING;
    struct xdma_engine *engine;
    struct engine_regs *regs;
    uint32_t channel_id, engine_id;

    // H2C
    if (dir == 0)
    {
        engine = &(xdev->engine_h2c[channel]);
    }
    else
    {
        offset += H2C_CHANNEL_OFFSET;
        engine = &(xdev->engine_c2h[channel]);
    }


    // get config basse addr to init fpga dma engine
    regs = xdev->bar[xdev->config_bar_idx] + offset;
    engine_id = get_engine_id(regs);
    channel_id = get_engine_channel_id(regs);

    Debug_log("found AXI %s %d engine, reg. off 0x%x, id 0x%x,0x%x.\n",
              dir == 0 ? "H2C" : "C2H",
              channel,
              offset,
              engine_id,
              channel_id);

    /* allocate and initialize engine */
    rv = engine_init(engine, xdev, offset, dir, channel);
    if (rv != 0)
    {
        Debug_log("failed to create AXI %s %d engine.\n", dir == 0 ? "H2C" : "C2H", channel);
        return rv;
    }

    return 0;
}

static int engine_reg_dump(struct xdma_engine *engine)
{
    uint32_t w;

    if (!engine)
    {
        Debug_log("dma engine NULL\n");
        return -1;
    }

    w = read_register(&engine->regs->identifier);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (id).\n", engine->name, &engine->regs->identifier, w);
    w &= BLOCK_ID_MASK;
    if (w != BLOCK_ID_HEAD)
    {
        Debug_log("%s: engine id missing, 0x%08x exp. & 0x%x = 0x%x\n",
                  engine->name,
                  w,
                  BLOCK_ID_MASK,
                  BLOCK_ID_HEAD);
        return -1;
    }
    /* extra debugging; inspect complete engine set of registers */
    w = read_register(&engine->regs->status);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (status).\n", engine->name, &engine->regs->status, w);
    w = read_register(&engine->regs->control);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (control)\n", engine->name, &engine->regs->control, w);
    w = read_register(&engine->sgdma_regs->first_desc_lo);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (first_desc_lo)\n",
              engine->name,
              &engine->sgdma_regs->first_desc_lo,
              w);
    w = read_register(&engine->sgdma_regs->first_desc_hi);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (first_desc_hi)\n",
              engine->name,
              &engine->sgdma_regs->first_desc_hi,
              w);
    w = read_register(&engine->sgdma_regs->first_desc_adjacent);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (first_desc_adjacent).\n",
              engine->name,
              &engine->sgdma_regs->first_desc_adjacent,
              w);
    w = read_register(&engine->regs->completed_desc_count);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (completed_desc_count).\n",
              engine->name,
              &engine->regs->completed_desc_count,
              w);
    w = read_register(&engine->regs->interrupt_enable_mask);
    Debug_log("%s: ioread32(0x%p) = 0x%08x (interrupt_enable_mask)\n",
              engine->name,
              &engine->regs->interrupt_enable_mask,
              w);

    return 0;
}

static void engine_status_dump(struct xdma_engine *engine)
{
    uint32_t v = engine->status;
    char buffer[256];
    char *buf = buffer;
    int len;

    len = sprintf(buf, "SG engine %s status: 0x%08x: ", engine->name, v);

    if ((v & XDMA_STAT_BUSY))
        len += sprintf(buf + len, "BUSY,");
    if ((v & XDMA_STAT_DESC_STOPPED))
        len += sprintf(buf + len, "DESC_STOPPED,");
    if ((v & XDMA_STAT_DESC_COMPLETED))
        len += sprintf(buf + len, "DESC_COMPL,");

    /* common H2C & C2H */
    if ((v & XDMA_STAT_COMMON_ERR_MASK))
    {
        if ((v & XDMA_STAT_ALIGN_MISMATCH))
            len += sprintf(buf + len, "ALIGN_MISMATCH ");
        if ((v & XDMA_STAT_MAGIC_STOPPED))
            len += sprintf(buf + len, "MAGIC_STOPPED ");
        if ((v & XDMA_STAT_INVALID_LEN))
            len += sprintf(buf + len, "INVLIAD_LEN ");
        if ((v & XDMA_STAT_IDLE_STOPPED))
            len += sprintf(buf + len, "IDLE_STOPPED ");
        buf[len - 1] = ',';
    }

    if (engine->dir == 0)
    {
        /* H2C only */
        if ((v & XDMA_STAT_H2C_R_ERR_MASK))
        {
            len += sprintf(buf + len, "R:");
            if ((v & XDMA_STAT_H2C_R_UNSUPP_REQ))
                len += sprintf(buf + len, "UNSUPP_REQ ");
            if ((v & XDMA_STAT_H2C_R_COMPL_ABORT))
                len += sprintf(buf + len, "COMPL_ABORT ");
            if ((v & XDMA_STAT_H2C_R_PARITY_ERR))
                len += sprintf(buf + len, "PARITY ");
            if ((v & XDMA_STAT_H2C_R_HEADER_EP))
                len += sprintf(buf + len, "HEADER_EP ");
            if ((v & XDMA_STAT_H2C_R_UNEXP_COMPL))
                len += sprintf(buf + len, "UNEXP_COMPL ");
            buf[len - 1] = ',';
        }

        if ((v & XDMA_STAT_H2C_W_ERR_MASK))
        {
            len += sprintf(buf + len, "W:");
            if ((v & XDMA_STAT_H2C_W_DECODE_ERR))
                len += sprintf(buf + len, "DECODE_ERR ");
            if ((v & XDMA_STAT_H2C_W_SLAVE_ERR))
                len += sprintf(buf + len, "SLAVE_ERR ");
            buf[len - 1] = ',';
        }
    }
    else
    {
        /* C2H only */
        if ((v & XDMA_STAT_C2H_R_ERR_MASK))
        {
            len += sprintf(buf + len, "R:");
            if ((v & XDMA_STAT_C2H_R_DECODE_ERR))
                len += sprintf(buf + len, "DECODE_ERR ");
            if ((v & XDMA_STAT_C2H_R_SLAVE_ERR))
                len += sprintf(buf + len, "SLAVE_ERR ");
            buf[len - 1] = ',';
        }
    }

    /* common H2C & C2H */
    if ((v & XDMA_STAT_DESC_ERR_MASK))
    {
        len += sprintf(buf + len, "DESC_ERR:");
        if ((v & XDMA_STAT_DESC_UNSUPP_REQ))
            len += sprintf(buf + len, "UNSUPP_REQ ");
        if ((v & XDMA_STAT_DESC_COMPL_ABORT))
            len += sprintf(buf + len, "COMPL_ABORT ");
        if ((v & XDMA_STAT_DESC_PARITY_ERR))
            len += sprintf(buf + len, "PARITY ");
        if ((v & XDMA_STAT_DESC_HEADER_EP))
            len += sprintf(buf + len, "HEADER_EP ");
        if ((v & XDMA_STAT_DESC_UNEXP_COMPL))
            len += sprintf(buf + len, "UNEXP_COMPL ");
        buf[len - 1] = ',';
    }

    buf[len - 1] = '\0';
    Debug_log("%s\n", buffer);
}

static int engine_status_read(struct xdma_engine *engine, bool clear, bool dump)
{
    int rv = 0;

    if (!engine)
    {
        Debug_log("dma engine NULL\n");
        return -1;
    }

    if (dump)
    {
        rv = engine_reg_dump(engine);
        if (rv < 0)
        {
            Debug_log("Failed to dump register\n");
            return rv;
        }
    }

    /* read status register */
    if (clear)
        engine->status = read_register(&engine->regs->status_rc);
    else
        engine->status = read_register(&engine->regs->status);

    if (dump)
        engine_status_dump(engine);

    return rv;
}

static int engine_start_mode_config(struct xdma_engine *engine)
{
    uint32_t w;

    /* write control register of SG DMA engine */
    w = (uint32_t)XDMA_CTRL_RUN_STOP;
    w |= (uint32_t)XDMA_CTRL_IE_READ_ERROR;
    w |= (uint32_t)XDMA_CTRL_IE_DESC_ERROR;
    w |= (uint32_t)XDMA_CTRL_IE_DESC_ALIGN_MISMATCH;
    w |= (uint32_t)XDMA_CTRL_IE_MAGIC_STOPPED;

    w |= (uint32_t)XDMA_CTRL_IE_DESC_STOPPED;
    w |= (uint32_t)XDMA_CTRL_IE_DESC_COMPLETED;

    Debug_log("iowrite32(0x%08x to 0x%p) (control)\n", w,
              (void *)&engine->regs->control);

    /* start the engine */
    write_register(w, &engine->regs->control);

    /* dummy read of status register to flush all previous writes */
    w = read_register(&engine->regs->status);
    Debug_log("ioread32(0x%p) = 0x%08x (dummy read flushes writes).\n",
              &engine->regs->status, w);
    return 0;
}

static int setup_xdma_desc(struct xdma_engine *engine, uintptr_t sg_desc_addr)
{
    uint32_t w, next_adj;

    if (!engine)
    {
        Debug_log("dma engine NULL\n");
        return -1;
    }

    /* engine must be idle */
    if (engine->running)
    {
        Debug_log("%s engine is not in idle state to start\n", engine->name);
        return -1;
    }

    /* write lower 32-bit of bus address of transfer first descriptor */
    w = PCI_DMA_L(sg_desc_addr);
    Debug_log("iowrite32(0x%08x to 0x%p) (first_desc_lo)\n",
              w,
              (void *)&engine->sgdma_regs->first_desc_lo);
    write_register(w, &engine->sgdma_regs->first_desc_lo);
    /* write upper 32-bit of bus address of transfer first descriptor */
    w = PCI_DMA_H(sg_desc_addr);
    Debug_log("iowrite32(0x%08x to 0x%p) (first_desc_hi)\n",
              w,
              (void *)&engine->sgdma_regs->first_desc_hi);
    write_register(w, &engine->sgdma_regs->first_desc_hi);

    /* no linked descriptor*/
    next_adj = 0;
    write_register(next_adj, &engine->sgdma_regs->first_desc_adjacent);

    Debug_log("ioread32(0x%p) (dummy read flushes writes).\n",
              &engine->regs->status);

    engine_start_mode_config(engine);

    /* add not passed */
#if 0
    rv = engine_status_read(engine, 0, 1);
    if (rv < 0)
    {
        Debug_log("Failed to read engine status\n");
        return -1;
    }
    Debug_log("%s engine 0x%p now running\n", engine->name, engine);
#endif
    engine->running = 0;
    return 0;
}

static void xdma_init(void)
{
    uint32_t i, rv;

    /* iterate over channels */
    for (i = 0; i < xdma_dev_inst.h2c_channel_max; i++)
    {
        rv = probe_for_engine(&xdma_dev_inst, 0, i);
        if (rv)
            break;
    }
    xdma_dev_inst.h2c_channel_max = i;

    /* iterate over channels */
    for (i = 0; i < xdma_dev_inst.c2h_channel_max; i++)
    {
        rv = probe_for_engine(&xdma_dev_inst, 1, i);
        if (rv)
            break;
    }
    xdma_dev_inst.c2h_channel_max = i;
}

int xdma_desc_h2c_init(int len)
{
    uint32_t control;

    if (len < 0)
        return -1;

    /* stop engine, EOP for AXI ST, req IRQ on last descriptor */
    control = XDMA_DESC_STOPPED;
    control |= DESC_MAGIC;
    control |= XDMA_DESC_EOP;
    control |= XDMA_DESC_COMPLETED;
    // DMA_TO_DEVICE
    ib_buff_space.xdma_sg_desc_h2c[0].next_lo = PCI_DMA_L(0);
    ib_buff_space.xdma_sg_desc_h2c[0].next_lo = PCI_DMA_H(0);
    ib_buff_space.xdma_sg_desc_h2c[0].bytes = len;

    /* read from root complex memory (source address) */
    /* pcie addr */
    ib_buff_space.xdma_sg_desc_h2c[0].src_addr_lo = PCI_DMA_L(TX_BUFF_ADDR);
    ib_buff_space.xdma_sg_desc_h2c[0].src_addr_hi = PCI_DMA_H(TX_BUFF_ADDR);

    /* write to end point address (destination address) */
    /* fpga axi addr */
    ib_buff_space.xdma_sg_desc_h2c[0].dst_addr_lo = PCI_DMA_L(AXI_ADDRESS);
    ib_buff_space.xdma_sg_desc_h2c[0].dst_addr_hi = PCI_DMA_H(AXI_ADDRESS);

    ib_buff_space.xdma_sg_desc_h2c[0].control = control;

    return 0;
}

int xdma_desc_c2h_init(int len)
{
    uint32_t control;

    if (len < 0)
        return -1;

    /* stop engine, EOP for AXI ST, req IRQ on last descriptor */
    control = XDMA_DESC_STOPPED;
    control |= DESC_MAGIC;
    control |= XDMA_DESC_EOP;
    control |= XDMA_DESC_COMPLETED;
    // DMA_TO_DEVICE
    ib_buff_space.xdma_sg_desc_c2h[0].next_lo = PCI_DMA_L(0);
    ib_buff_space.xdma_sg_desc_c2h[0].next_lo = PCI_DMA_H(0);
    ib_buff_space.xdma_sg_desc_c2h[0].bytes = len;

    /* read from root complex memory (source address) */
    /* fpga ram axi addr */
    ib_buff_space.xdma_sg_desc_c2h[0].src_addr_lo = PCI_DMA_L(AXI_ADDRESS);
    ib_buff_space.xdma_sg_desc_c2h[0].src_addr_hi = PCI_DMA_H(AXI_ADDRESS);

    /* write to end point address (destination address) */
    /* pcie addr */
    ib_buff_space.xdma_sg_desc_c2h[0].dst_addr_lo = PCI_DMA_L(RX_BUFF_ADDR);
    ib_buff_space.xdma_sg_desc_c2h[0].dst_addr_hi = PCI_DMA_H(RX_BUFF_ADDR);
    ib_buff_space.xdma_sg_desc_c2h[0].control = control;

    return 0;
}

int process_data(int dir)
{
    int status;
    // H2C
    if (dir == XDMA_TO_DEVICE)
    {
        status = setup_xdma_desc(&(xdma_dev_inst.engine_h2c[0]), H2C_DESC_ADDR);
    }
    else if (dir == XDMA_FROM_DEVICE)
    {
        status = setup_xdma_desc(&(xdma_dev_inst.engine_c2h[0]), C2H_DESC_ADDR);
    }
    else
    {
        Debug_log("do not processing of data\n");
        status = -1;
    }

    return status;
}


void packet_init(void)
{
    int Index;
    uint32_t Value;

    /* Create pattern in the packet to transmit */
    Value = 0;

    for (Index = 0; Index < MAX_PKT_LEN; Index++)
    {
        ib_buff_space.rx_buffer[Index] = Value;
    }

    for (Index = 0; Index < MAX_PKT_LEN; Index++)
    {
        ib_buff_space.tx_buffer[Index] = Value;

        Value = (Value + 1) & 0xFFFF;
    }
}

void pcie_xdma()
{
    xdma_dev_inst.idx = 1;
    xdma_dev_inst.h2c_channel_max = XDMA_CHANNEL_NUM_MAX;
    xdma_dev_inst.c2h_channel_max = XDMA_CHANNEL_NUM_MAX;

    /*config irq*/
    xdma_dev_inst.msi_enabled = 0;
    xdma_dev_inst.legacy_enabled = 0;  // enable legacy:1 disable legacy:0

    // fpga pcie to axi-lite master bar
    xdma_dev_inst.user_bar_idx = 0;
    xdma_dev_inst.bar[0] = (void *)PCIE0_AXI_LITE_BASE;

    // fpga dma bar
    xdma_dev_inst.config_bar_idx = 1;
    xdma_dev_inst.bar[1] = (void *)PCIE0_XDMA_BASE;

//    packet_init();

//    xdma_desc_h2c_init(H2C_DMA_LEN);
//    xdma_desc_c2h_init(C2H_DMA_LEN);
    is_config_bar(&xdma_dev_inst, xdma_dev_inst.config_bar_idx);
    xdma_init();
}



