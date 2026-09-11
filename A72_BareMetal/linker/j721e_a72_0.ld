OUTPUT_ARCH(aarch64)
ENTRY(Entry)

/*
 * TI-compatible A72/ATF layout.
 *
 *   0x70000000..0x7001ffff : reserved for TI ATF BL31 (EL3)
 *   0x70020000..            : existing R5F/other-core MSMC allocations
 *   0x80080000..            : non-secure A72 bare-metal BL33 (EL1),
 *                             including vectors, MMU tables and stacks
 *
 * 0x80080000 is the BL33 address expected by the prebuilt TI J721E BL31.
 */
MEMORY
{
    ATF_BL31   (rx)  : ORIGIN = 0x70000000, LENGTH = 0x00020000
    A72_DDR    (rwx) : ORIGIN = 0x80080000, LENGTH = 0x01F80000
}

SECTIONS
{
    . = ORIGIN(A72_DDR);
    __ddr_image_start = .;

    .boot : ALIGN(128)
    {
        KEEP(*(.text.boot))
    } > A72_DDR

    .vectors : ALIGN(2048)
    {
        KEEP(*(.vecs))
    } > A72_DDR

    .text : ALIGN(128)
    {
        *(.text .text.*)
    } > A72_DDR

    .rodata : ALIGN(16)
    {
        *(.rodata .rodata.*)
    } > A72_DDR

    .data : ALIGN(16)
    {
        __data_load = LOADADDR(.data);
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } > A72_DDR

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(16);
        __bss_end = .;
    } > A72_DDR

    /* L1 plus two L2 tables: low peripherals and the DDR IPC carveout. */
    .mmu_tables (NOLOAD) : ALIGN(4096)
    {
        __mmu_l1_table = .;
        . += 0x1000;
        __mmu_l2_table = .;
        . += 0x1000;
        __mmu_l2_ddr_table = .;
        . += 0x1000;
        __mmu_tables_end = .;
    } > A72_DDR

    .boot_stack (NOLOAD) : ALIGN(64)
    {
        __boot_stack_bottom = .;
        . += 0x10000;
        __boot_stack_top = .;
    } > A72_DDR

    .stack (NOLOAD) : ALIGN(64)
    {
        __stack_bottom = .;
        . += 0x20000;
        __stack_top = .;
    } > A72_DDR

    /* Heap used by the A72 GCC/newlib libc malloc family. */
    .heap (NOLOAD) : ALIGN(16)
    {
        __heap_start = .;
        . += 0x00200000;
        __heap_end = .;
    } > A72_DDR

    __ddr_image_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

ASSERT((ADDR(.vectors) & 0x7ff) == 0,
       "A72 EL1 vector table is not 2 KiB aligned")
ASSERT(__ddr_image_end <= ORIGIN(A72_DDR) + LENGTH(A72_DDR),
       "A72 BL33 image exceeds its reserved DDR window")
