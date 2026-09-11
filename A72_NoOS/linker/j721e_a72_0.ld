OUTPUT_ARCH(aarch64)
ENTRY(_start)

/*
 * The existing CPHMC TBL clears 0x70000000..0x706fffff before stage-2 load.
 * This address is J721E MSMC SRAM, not external DDR. Keep the No-OS image
 * inside the 7 MiB window already reserved by the CPHMC boot flow.
 */
MEMORY
{
    A72_MSMC (rwx) : ORIGIN = 0x70000000, LENGTH = 0x00700000
}

SECTIONS
{
    . = ORIGIN(A72_MSMC);
    __image_start = .;

    .text : ALIGN(128)
    {
        KEEP(*(.text.boot))
        *(.text .text.*)
    } > A72_MSMC

    .vectors : ALIGN(2048)
    {
        KEEP(*(.vectors))
    } > A72_MSMC

    .rodata : ALIGN(16)
    {
        *(.rodata .rodata.*)
    } > A72_MSMC

    .data : ALIGN(16)
    {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } > A72_MSMC

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_start = .;
        *(.bss .bss.* COMMON)
        . = ALIGN(16);
        __bss_end = .;
    } > A72_MSMC

    .stack (NOLOAD) : ALIGN(64)
    {
        __stack_bottom = .;
        . += 0x00010000;
        __stack_top = .;
    } > A72_MSMC

    __image_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}

ASSERT(__image_end <= ORIGIN(A72_MSMC) + LENGTH(A72_MSMC),
       "A72 No-OS image exceeds the reserved 7 MiB MSMC SRAM window")
