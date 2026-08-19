
/*=========================*/
/*     Linker Settings     */
/*=========================*/

--retain="*(.bootCode)"
--retain="*(.startupCode)"
--retain="*(.startupData)"
--retain="*(.irqStack)"
--retain="*(.fiqStack)"
--retain="*(.abortStack)"
--retain="*(.undStack)"
--retain="*(.svcStack)"

--fill_value=0
--stack_size=0x8000
--heap_size=0x8000

--entry_point=_freertosresetvectors

-stack  0x8000  /* SOFTWARE STACK SIZE */
-heap   0x8000  /* HEAP AREA SIZE      */

/*-------------------------------------------*/
/*       Stack Sizes for various modes       */
/*-------------------------------------------*/
__IRQ_STACK_SIZE   = 0x1000;
__FIQ_STACK_SIZE   = 0x0100;
__ABORT_STACK_SIZE = 0x0100;
__UND_STACK_SIZE   = 0x0100;
__SVC_STACK_SIZE   = 0x0100;

/*--------------------------------------------------------------------------*/
/*                               Memory Map                                 */
/*--------------------------------------------------------------------------*/
MEMORY
{
    /*===================== C7x / R5F shared SRAM Locations ========================*/
    /* Core 7 is MCU R5F0 */
}

/*=========================*/
/*     Core 7 Mapping      */
/*=========================*/

#define __VECS                  MCU_R5F_TCMA_VECS
#define __BOOT                  MCU_R5F_TCMA
#define __CORE_IPC_DATA         MCU1_0_IPC_DATA
#define __CORE_RES              MCU1_0_RES_DATA
#define __CORE_RES_DATA         MCU1_0_RES_DATA
#define __CORE_SPACE            MCU1_0_DDR_SPACE
#define __CORE_DDR_SPACE        MCU1_0_DDR_SPACE

/*--------------------------------------------------------------*/
/*                     Section Configuration                    */
/*--------------------------------------------------------------*/
SECTIONS
{
    .freertosrstvectors      : {} palign(8)      > MCU_R5F_TCMA_VECS
    .bootCode                : {} palign(8)      > OCMC_RAM
    .startupCode             : {} palign(8)      > OCMC_RAM
    .startupData             : {} palign(8)      > OCMC_RAM, type = NOINIT

    .text : {
         __start_text = .;
         *(.text*)
         __end_text = .;
    } palign(8) > OCMC_RAM

    GROUP {
        .text.hwi    : palign(8)
        .text.cache  : palign(8)
        .text.mpu    : palign(8)
        .text.boot   : palign(8)
    } > OCMC_RAM

    .const           : {} palign(8)      > __CORE_DDR_SPACE
    .rodata          : {} palign(8)      > __CORE_DDR_SPACE
    .cinit           : {} palign(8)      > __CORE_DDR_SPACE
    .bss             : {} align(4)       > OCMC_RAM_SBL_RUNTIME
    .far             : {} align(4)       > OCMC_RAM
    .data            : {} palign(128)    > __CORE_DDR_SPACE
    .sysmem          : {}                > OCMC_RAM
    .boardcfg_data   : {} align(4)       > OCMC_RAM

    .stack      : {} align(4)                               > OCMC_RAM  (HIGH)

    .irqStack   : {. = . + __IRQ_STACK_SIZE;} align(4)      > OCMC_RAM  (HIGH)
    RUN_START(__IRQ_STACK_START)
    RUN_END(__IRQ_STACK_END)

    .fiqStack   : {. = . + __FIQ_STACK_SIZE;} align(4)      > OCMC_RAM  (HIGH)
    RUN_START(__FIQ_STACK_START)
    RUN_END(__FIQ_STACK_END)

    .abortStack : {. = . + __ABORT_STACK_SIZE;} align(4)    > OCMC_RAM  (HIGH)
    RUN_START(__ABORT_STACK_START)
    RUN_END(__ABORT_STACK_END)

    .undStack   : {. = . + __UND_STACK_SIZE;} align(4)      > OCMC_RAM  (HIGH)
    RUN_START(__UND_STACK_START)
    RUN_END(__UND_STACK_END)

    .svcStack   : {. = . + __SVC_STACK_SIZE;} align(4)      > OCMC_RAM  (HIGH)
	.ipc_data_buffer (NOINIT) : {} palign(128) > __CORE_DDR_SPACE
	.resource_table :{ __RESOURCE_TABLE = .;} > __CORE_RES
	.tracebuf : {} align(1024) > __CORE_RES_DATA
	    RUN_START(__SVC_STACK_START)
    RUN_END(__SVC_STACK_END)
}
