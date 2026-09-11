/*
 ******************************************************************************
  * @copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
  * @file       startup.S
  * @author     zhaoguodong
  * @date       2026.09.11
  * @brief      A72 No-OS startup and exception-vector setup.
  * @par        History
  *Date        Version   Author         Description
  *2026.09.11  1.0       zhaoguodong    Initial version
 ******************************************************************************
 * J721E Cortex-A72_0 bare-metal startup, adapted from TI PDK csl_init.
 * Entry at EL3 falls through EL2h to EL1h; EL2 enters EL1h directly; an
 * ATF-provided EL1 entry continues without touching EL3. Application code,
 * exception vectors, MMU and caches always run at EL1.
 */

    .equ J721E_GICD_BASE,        0x01800000
    .equ J721E_GICR_BASE_CORE0, 0x01900000
    .equ J721E_GICR_SGI_CORE0,  0x01910000
    .equ J721E_GTC_CNTCR,       0x00a90000

    .section .text.boot, "ax"
    .align  7
    .global Entry
    .global _start
    .type   Entry, %function
Entry:
_start:
    msr     daifset, #0xf
    /* Remember whether this image itself owns the EL3-to-EL1 transition. */
    mov     x19, xzr

    /* This image owns only Cortex-A72_0. */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xffff
    cbnz    x0, .Lpark

    mrs     x0, CurrentEL
    cmp     x0, #0xc
    b.ne    .Lcheck_el2
    mov     x19, #1

    /*
     * J721E A72 power-domain requirement from TI bootcode.asm.
     * This implementation-defined register is accessible on the direct EL3
     * boot path.  When ATF hands BL33 to EL1, EL3 firmware already owns this
     * setup and an EL1 write can trap back into the secure monitor.
     */
    mrs     x0, S3_1_c11_c0_2
    orr     x0, x0, #(1 << 1)
    msr     S3_1_c11_c0_2, x0

    /* EL3 -> non-secure AArch64 EL2h. */
    bl      a72_disable_el3_caches
    mov     x0, #0x0531
    msr     scr_el3, x0
    msr     cptr_el3, xzr
    mrs     x0, actlr_el3
    orr     x0, x0, #0x2
    msr     actlr_el3, x0

    /* TI GICv3 security/group initialization. */
    ldr     x0, =J721E_GICD_BASE
    mov     w1, #0x37
    str     w1, [x0]
    mov     x1, #0xf
    msr     s3_6_c12_c12_5, x1      /* ICC_SRE_EL3 */
    isb
    mov     x1, #1
    msr     s3_6_c12_c12_7, x1      /* ICC_IGRPEN1_EL3 */
    isb

    add     x2, x0, #0x80
    mov     w1, #-1
    mov     x3, #32
.Lgicd_group_loop:
    str     w1, [x2], #4
    subs    x3, x3, #1
    b.ne    .Lgicd_group_loop
    add     x2, x0, #0xd00
    mov     w1, wzr
    mov     x3, #32
.Lgicd_mod_loop:
    str     w1, [x2], #4
    subs    x3, x3, #1
    b.ne    .Lgicd_mod_loop

    ldr     x0, =J721E_GICR_BASE_CORE0
    ldr     w1, [x0, #0x14]
    bic     w1, w1, #0x2
    str     w1, [x0, #0x14]
.Lgicr_wake_wait:
    ldr     w1, [x0, #0x14]
    tst     w1, #0x4
    b.ne    .Lgicr_wake_wait
    ldr     x0, =J721E_GICR_SGI_CORE0
    mov     w1, #-1
    str     w1, [x0, #0x80]
    str     wzr, [x0, #0xd00]

    mov     x0, #0x0830
    movk    x0, #0x30c5, lsl #16
    msr     sctlr_el2, x0

    ldr     x0, =J721E_GTC_CNTCR
    ldr     w1, [x0]
    orr     w1, w1, #1
    str     w1, [x0]
    dsb     sy

    adr     x0, .Lentered_el2
    msr     elr_el3, x0
    mov     x0, #0x03c9             /* EL2h, DAIF masked */
    msr     spsr_el3, x0
    isb
    eret

.Lcheck_el2:
    cmp     x0, #0x8
    b.eq    .Lentered_el2
    cmp     x0, #0x4
    b.eq    .Lentered_el1
    b       .Lpark

.Lentered_el2:
    /* EL2 -> non-secure AArch64 EL1h. */
    mrs     x0, cnthctl_el2
    orr     x0, x0, #0x3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    mov     x0, #0x33ff
    msr     cptr_el2, x0
    msr     hstr_el2, xzr
    mov     x0, #0x0002
    movk    x0, #0xa000, lsl #16
    msr     hcr_el2, x0
    mov     x0, #0x0838
    movk    x0, #0x30d0, lsl #16
    msr     sctlr_el1, x0
    mrs     x0, actlr_el2
    orr     x0, x0, #0x2
    msr     actlr_el2, x0
    mov     x0, #0xf
    msr     s3_4_c12_c9_5, x0       /* ICC_SRE_EL2 */
    isb

    adr     x0, .Lentered_el1
    msr     elr_el2, x0
    mov     x0, #0x03c5             /* EL1h, DAIF masked */
    msr     spsr_el2, x0
    isb
    eret

.Lentered_el1:
    msr     daifset, #0xf
    msr     spsel, #1
    mov     x0, #(3 << 20)
    msr     cpacr_el1, x0
    isb
    ldr     x0, =__boot_stack_top
    mov     sp, x0
    /* Use TI CSL's AArch64 exception vectors and IRQ dispatcher. */
    ldr     x0, =ti_csl_arm_gicv3_vectors
    msr     vbar_el1, x0
    isb
    b       cphmc_start_boot
    .size   Entry, . - Entry

/* TI-style runtime hand-off: BSS, coherency, MMU/cache, stack, main. */
    .align 4
    .global cphmc_start_boot
    .type   cphmc_start_boot, %function
cphmc_start_boot:
    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
.Lbss_loop:
    cmp     x0, x1
    b.hs    .Lbss_done
    str     xzr, [x0], #8
    b       .Lbss_loop
.Lbss_done:
    dsb     sy
    bl      a72_mmu_cache_enable_el1
    ldr     x0, =__stack_top
    mov     sp, x0
    ldr     x0, =a72_main
    blr     x0
    b       .Lpark

.Lpark:
    wfe
    b       .Lpark
    .size   cphmc_start_boot, . - cphmc_start_boot

/*
 * EL1 stage-1 identity map, based on TI CSL_mmuStartup(), but retaining our
 * planned A72 DDR placement for translation tables and application data.
 */
    .align  4
    .type   a72_mmu_cache_enable_el1, %function
a72_mmu_cache_enable_el1:
    stp     x29, x30, [sp, #-16]!
    mrs     x0, sctlr_el1
    mov     x1, #0x1005
    bic     x0, x0, x1
    msr     sctlr_el1, x0
    isb
    bl      a72_invalidate_dcache_all

    /*
     * CPUECTLR_EL1.SMPEN, equivalent to
     * CSL_a53v8EnableDataCoherency().  TI ATF already sets SMPEN before
     * entering BL33 (the register reads back with bit 6 set), but prevents
     * non-secure EL1 from writing this implementation-defined register.
     * Only repeat the write when this startup code performed the EL3 setup
     * itself and enabled the lower-EL access in ACTLR_EL3.
     */
    cbz     x19, .Lcoherency_ready
    mrs     x0, S3_1_c15_c2_1
    orr     x0, x0, #0x40
    msr     S3_1_c15_c2_1, x0
.Lcoherency_ready:
    isb

    ldr     x0, =__mmu_l1_table
    mov     x1, #1536
.Lclear_tables:
    str     xzr, [x0], #8
    subs    x1, x1, #1
    b.ne    .Lclear_tables

    ldr     x0, =__mmu_l1_table
    ldr     x1, =__mmu_l2_table
    /* AttrIdx0 Device, outer-shareable, AF, PXN/UXN. */
    ldr     x2, =0x0060000000000601
    str     x2, [x0, #0]
    orr     x3, x1, #0x3
    str     x3, [x0, #8]
    /* 0x80000000..0xbfffffff uses an L2 table for the IPC carveout. */
    ldr     x3, =__mmu_l2_ddr_table
    orr     x3, x3, #0x3
    str     x3, [x0, #16]
    /* AttrIdx1 Normal WBWA, outer-shareable, AF, UXN but EL1 executable. */
    ldr     x3, =0x00400000c0000605
    str     x3, [x0, #24]

    /* TI default high-address device windows. */
    ldr     x3, =0x0060000400000601
    str     x3, [x0, #(16 * 8)]
    ldr     x3, =0x0060000600000601
    str     x3, [x0, #(24 * 8)]
    ldr     x3, =0x0060004d00000601
    str     x3, [x0, #(308 * 8)]

    /* 0x40000000..0x6fffffff: Device in 2 MiB blocks. */
    mov     x3, #0x40000000
    mov     x4, #384
.Lfill_l2_device:
    orr     x5, x3, x2
    str     x5, [x1], #8
    add     x3, x3, #0x200000
    subs    x4, x4, #1
    b.ne    .Lfill_l2_device

    /* 0x70000000..0x7fffffff: Normal WBWA, EL1 executable. */
    ldr     x2, =0x0040000000000605
    mov     x4, #128
.Lfill_l2_normal:
    orr     x5, x3, x2
    str     x5, [x1], #8
    add     x3, x3, #0x200000
    subs    x4, x4, #1
    b.ne    .Lfill_l2_normal

    /* 0x80000000..0xbfffffff: Normal WBWA in 2 MiB blocks. */
    ldr     x1, =__mmu_l2_ddr_table
    ldr     x2, =0x0040000000000605
    mov     x3, #0x80000000
    mov     x4, #512
.Lfill_l2_ddr:
    orr     x5, x3, x2
    str     x5, [x1], #8
    add     x3, x3, #0x200000
    subs    x4, x4, #1
    b.ne    .Lfill_l2_ddr

    /*
     * 0xAA000000..0xABFFFFFF: IPC VRING shared with C7X.
     * AttrIdx2 is Normal Non-cacheable so both directions observe DDR
     * immediately; PXN/UXN prevents execution from the shared buffer.
     */
    ldr     x1, =__mmu_l2_ddr_table
    add     x1, x1, #0xa80
    ldr     x2, =0x0060000000000609
    mov     x3, #0xaa000000
    mov     x4, #16
.Lfill_l2_ipc:
    orr     x5, x3, x2
    str     x5, [x1], #8
    add     x3, x3, #0x200000
    subs    x4, x4, #1
    b.ne    .Lfill_l2_ipc
    dsb     sy

    /* MAIR0 Device, MAIR1 Normal WBWA, MAIR2 Normal Non-cacheable. */
    mov     x0, #0xff00
    movk    x0, #0x44, lsl #16
    msr     mair_el1, x0
    ldr     x0, =0x0000000500000019 /* TI CSL TCR_EL1 value */
    msr     tcr_el1, x0
    ldr     x0, =__mmu_l1_table
    msr     ttbr0_el1, x0
    dsb     sy
    tlbi    vmalle1
    dsb     sy
    isb
    ic      iallu
    dsb     sy
    isb
    mrs     x0, sctlr_el1
    mov     x1, #0x1005
    orr     x0, x0, x1
    msr     sctlr_el1, x0
    isb
    ldp     x29, x30, [sp], #16
    ret
    .size   a72_mmu_cache_enable_el1, . - a72_mmu_cache_enable_el1

/* TI-derived cache maintenance used before changing translation state. */
    .align  4
    .type   a72_disable_el3_caches, %function
a72_disable_el3_caches:
    mov     x14, x30
    bl      a72_clean_invalidate_dcache_all
    mov     x30, x14
    mrs     x0, sctlr_el3
    mov     x1, #0x1004
    bic     x0, x0, x1
    msr     sctlr_el3, x0
    ic      iallu
    dsb     sy
    isb
    ret

    .align  4
    .type   a72_invalidate_dcache_all, %function
a72_invalidate_dcache_all:
    mov     x13, #0
    b       a72_dcache_all

    .type   a72_clean_invalidate_dcache_all, %function
a72_clean_invalidate_dcache_all:
    mov     x13, #1
a72_dcache_all:
    mrs     x0, clidr_el1
    ubfx    x1, x0, #24, #3
    cbz     x1, .Ldcache_done
    mov     x2, #0
.Ldcache_level:
    add     x3, x2, x2, lsl #1
    lsr     x4, x0, x3
    and     x4, x4, #0x7
    cmp     x4, #2
    b.lt    .Ldcache_next_level
    lsl     x9, x2, #1
    msr     csselr_el1, x9
    isb
    mrs     x4, ccsidr_el1
    and     x5, x4, #0x7
    add     x5, x5, #4
    ubfx    x6, x4, #3, #10
    clz     w7, w6
    ubfx    x8, x4, #13, #15
.Ldcache_way:
    mov     x10, x8
.Ldcache_set:
    lsl     x11, x6, x7
    lsl     x12, x10, x5
    orr     x11, x11, x12
    orr     x11, x11, x9
    cbnz    x13, .Ldcache_cisw
    dc      isw, x11
    b       .Ldcache_op_done
.Ldcache_cisw:
    dc      cisw, x11
.Ldcache_op_done:
    subs    x10, x10, #1
    b.ge    .Ldcache_set
    subs    x6, x6, #1
    b.ge    .Ldcache_way
.Ldcache_next_level:
    add     x2, x2, #1
    cmp     x2, x1
    b.lt    .Ldcache_level
.Ldcache_done:
    msr     csselr_el1, xzr
    dsb     sy
    isb
    ret

    .section .note.GNU-stack, "", %progbits
