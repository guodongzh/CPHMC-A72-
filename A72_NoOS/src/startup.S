/* Minimal AArch64 reset and exception entry for J721E Cortex-A72_0. */

    .section .text.boot, "ax"
    .align  7
    .global _start
    .type   _start, %function
_start:
    msr     daifset, #0xf

    /* Only Aff0=0/Aff1=0 is allowed to run this single-core image. */
    mrs     x0, mpidr_el1
    and     x0, x0, #0xffff
    cbnz    x0, .Lpark

    ldr     x0, =__stack_top
    mov     sp, x0

    ldr     x0, =a72_vectors
    mrs     x1, CurrentEL
    cmp     x1, #0xc
    b.eq    .Lset_vbar_el3
    cmp     x1, #0x8
    b.eq    .Lset_vbar_el2
    cmp     x1, #0x4
    b.eq    .Lset_vbar_el1
    b       .Lpark

.Lset_vbar_el3:
    msr     vbar_el3, x0
    b       .Lvbar_done
.Lset_vbar_el2:
    msr     vbar_el2, x0
    b       .Lvbar_done
.Lset_vbar_el1:
    msr     vbar_el1, x0
.Lvbar_done:
    isb

    ldr     x0, =__bss_start
    ldr     x1, =__bss_end
.Lbss_loop:
    cmp     x0, x1
    b.hs    .Lbss_done
    str     xzr, [x0], #8
    b       .Lbss_loop
.Lbss_done:
    dsb     sy
    bl      a72_main

.Lpark:
    wfe
    b       .Lpark
    .size   _start, . - _start

    .section .vectors, "ax"
    .align  11
    .global a72_vectors
a72_vectors:

    .macro VECTOR_SLOT number
        mov     x0, #\number
        b       a72_exception_entry
        .balign 128, 0
    .endm

    VECTOR_SLOT 0
    VECTOR_SLOT 1
    VECTOR_SLOT 2
    VECTOR_SLOT 3
    VECTOR_SLOT 4
    VECTOR_SLOT 5
    VECTOR_SLOT 6
    VECTOR_SLOT 7
    VECTOR_SLOT 8
    VECTOR_SLOT 9
    VECTOR_SLOT 10
    VECTOR_SLOT 11
    VECTOR_SLOT 12
    VECTOR_SLOT 13
    VECTOR_SLOT 14
    VECTOR_SLOT 15

    .section .text, "ax"
    .align  4
    .type   a72_exception_entry, %function
a72_exception_entry:
    /* x0 is the vector slot. Exceptions are fatal in this bring-up image. */
    mrs     x5, CurrentEL
    lsr     x1, x5, #2
    cmp     x5, #0xc
    b.eq    .Lexc_el3
    cmp     x5, #0x8
    b.eq    .Lexc_el2

    mrs     x2, esr_el1
    mrs     x3, elr_el1
    mrs     x4, far_el1
    b       .Lexc_call
.Lexc_el2:
    mrs     x2, esr_el2
    mrs     x3, elr_el2
    mrs     x4, far_el2
    b       .Lexc_call
.Lexc_el3:
    mrs     x2, esr_el3
    mrs     x3, elr_el3
    mrs     x4, far_el3
.Lexc_call:
    bl      a72_exception_handler
    b       .Lpark
    .size   a72_exception_entry, . - a72_exception_entry

    .section .note.GNU-stack, "", %progbits
