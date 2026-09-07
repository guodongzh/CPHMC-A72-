#include "memory_map_defines.h"

/*=========================*/
/*     Linker Settings     */
/*=========================*/

#if (Core == 0) || (Core == 1) || (Core == 2) || (Core == 3) || (Core == 7)

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
--entry_point=_resetvectors
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

#elif (Core==4) || (Core==5)

-stack  0x2000      /* SOFTWARE STACK SIZE */
-heap   0x1000      /* HEAP AREA SIZE      */
-e _c_int00

/*--------------------------------------------------------------------------*/
/*                               Memory Map                                 */
/*--------------------------------------------------------------------------*/
MEMORY
{
    /*===================== C66 SRAM Locations ========================*/
    /* 256KB LOCAL L2/SRAM */
    L2SRAM_INT					: ORIGIN = 0x00800000 LENGTH = 0x80      /* 128 byte */
    L2SRAM                      : ORIGIN = 0x00800080 LENGTH = 0x31F80   /* 200k */

    L2SRAM_VAR                  : ORIGIN = 0x00832000 LENGTH = 0x6000    /* 24k */ 
    L2SRAM_PCIE0         (RWIX) : ORIGIN = 0x00838000 LENGTH = 0x4000    /* 16k */

    L1PSRAM                     : ORIGIN = 0x00E00000 LENGTH = 0x00008000   /* 32KB LOCAL L1P/SRAM */
    L1DSRAM                     : ORIGIN = 0x00F00000 LENGTH = 0x00008000   /* 32KB LOCAL L1D/SRAM */
}

#elif (Core==6)

--ram_model
-heap  0x200
-stack 0x2000
--args 0x1000
--diag_suppress=10068 /* "no matching section" */
--cinit_compression=off
-e _c_int00_secure


/*--------------------------------------------------------------------------*/
/*                               Memory Map                                 */
/*--------------------------------------------------------------------------*/
MEMORY
{
    /*===================== c7000 SRAM Locations ========================*/
	L2SRAM           (RWX)      : ORIGIN = 0x64800000 LENGTH = 0x0080000        /* 512KB LOCAL L2/SRAM */
    /*===================== c66x L2SRAM global Locations ========================*/
    L2SRAM_PCIE_C66X0   (RWIX)  : ORIGIN = 0x4D80838000 LENGTH = 0x4000      /* 16k */
}

#endif

#if (Core == 0)

#define __VECS                  MCU_R5F_TCMA_VECS
#define __BOOT                  MCU_R5F_TCMA
#define __CORE_IPC_DATA         MCU2_0_IPC_DATA
#define __CORE_RES              MCU2_0_RES_DATA_BASE
#define __CORE_RES_DATA         MCU2_0_RES_DATA
#define __CORE_SPACE            MCU2_0_DDR_SPACE
#define __CORE_DDR_SPACE        MCU2_0_DDR_SPACE

#elif (Core == 1)

#define __VECS                  MCU_R5F_TCMA_VECS
#define __BOOT                  MCU_R5F_TCMA
#define __CORE_IPC_DATA         MCU2_1_IPC_DATA
#define __CORE_RES              MCU2_1_RES_DATA_BASE
#define __CORE_RES_DATA         MCU2_1_RES_DATA
#define __CORE_SPACE            MCU2_1_DDR_SPACE
#define __CORE_DDR_SPACE        MCU2_1_DDR_SPACE

#elif (Core == 2)

#define __VECS                  MCU_R5F_TCMA_VECS
#define __BOOT                  MCU_R5F_TCMA
#define __CORE_IPC_DATA         MCU3_0_IPC_DATA
#define __CORE_RES              MCU3_0_RES_DATA_BASE
#define __CORE_RES_DATA         MCU3_0_RES_DATA
#define __CORE_SPACE            MCU3_0_DDR_SPACE
#define __CORE_DDR_SPACE        MCU3_0_DDR_SPACE

#elif (Core == 3)

#define __VECS                  MCU_R5F_TCMA_VECS
#define __BOOT                  MCU_R5F_TCMA
#define __CORE_IPC_DATA         MCU3_1_IPC_DATA
#define __CORE_RES              MCU3_1_RES_DATA_BASE
#define __CORE_RES_DATA         MCU3_1_RES_DATA
#define __CORE_SPACE            MCU3_1_DDR_SPACE
#define __CORE_DDR_SPACE        MCU3_1_DDR_SPACE

#elif (Core == 4)

#define __VECS                  L2SRAM_INT
#define __CORE_SPACE            L2SRAM
#define __CORE_RES              C66x1_RES_DATA_BASE
#define __CORE_RES_DATA         C66X1_RES_DATA
#define __CORE_DDR_SPACE        C66X1_DDR_SPACE

#elif (Core == 5)

#define __VECS                  L2SRAM_INT
#define __CORE_SPACE            L2SRAM
#define __CORE_RES              C66x2_RES_DATA_BASE
#define __CORE_RES_DATA         C66X2_RES_DATA
#define __CORE_DDR_SPACE        C66X2_DDR_SPACE

#elif (Core == 6)

#define __VECS                  MSMC_C71x_VEC
#define __CORE_SPACE            MSMC_C71x
#define __CORE_RES              C7x_1_RES_DATA_BASE
#define __CORE_RES_DATA         C7X_RES_DATA
#define __CORE_DDR_SPACE        C7X_DDR_SPACE

#elif (Core == 7)

#define __VECS                  MCU_R5F_TCMA_VECS
#define __BOOT                  MCU_R5F_TCMA
#define __CORE_IPC_DATA         MCU1_1_IPC_DATA
#define __CORE_RES              MCU1_1_RES_DATA_BASE
#define __CORE_RES_DATA         MCU1_1_RES_DATA
#define __CORE_SPACE            MCU1_1_DDR_SPACE
#define __CORE_DDR_SPACE        MCU1_1_DDR_SPACE

#endif


/*--------------------------------------------------------------*/
/*                     Section Configuration                    */
/*--------------------------------------------------------------*/
SECTIONS
{
#if (Core == 0) || (Core == 1) || (Core == 2) || (Core == 3) || (Core == 7)
    .rstvectors : {} palign(8)              > __VECS
    .bootCode           : {} palign(8)      > __BOOT
    .startupCode        : {} palign(8)      > __BOOT
    .startupData        : {} palign(8)      > __BOOT, type = NOINIT
    GROUP
    {
        .text.hwi       : palign(8)
        .text.cache     : palign(8)
        .text.mpu       : palign(8)
        .text.boot      : palign(8)
    }   > __BOOT

    /* This is where the stacks for different R5F modes go */
    GROUP {
        .irqstack: {. = . + __IRQ_STACK_SIZE;} align(8)
        RUN_START(__IRQ_STACK_START)
        RUN_END(__IRQ_STACK_END)
        .fiqstack: {. = . + __FIQ_STACK_SIZE;} align(8)
        RUN_START(__FIQ_STACK_START)
        RUN_END(__FIQ_STACK_END)
        .svcstack: {. = . + __SVC_STACK_SIZE;} align(8)
        RUN_START(__SVC_STACK_START)
        RUN_END(__SVC_STACK_END)
        .abortstack: {. = . + __ABORT_STACK_SIZE;} align(8)
        RUN_START(__ABORT_STACK_START)
        RUN_END(__ABORT_STACK_END)
        .undefinedstack: {. = . + __UND_STACK_SIZE;} align(8)
        RUN_START(__UND_STACK_START)
        RUN_END(__UND_STACK_END)
    } > __BOOT

    .text : {
         __start_text = .;
        *(.text*)
         __end_text = .;
    } palign(8) > __CORE_SPACE
    .const              : {} palign(8)      > __CORE_SPACE
    .rodata             : {} palign(8)      > __CORE_SPACE
    .cinit              : {} palign(8)      > __CORE_SPACE
    .bss                : {} palign(4)       > __CORE_SPACE
    .far                : {} palign(4)       > __CORE_SPACE
    .data               : {} palign(128)    > __CORE_SPACE
    .sysmem             : {}                > __CORE_SPACE
    .stack align(4) >   __CORE_SPACE  (HIGH)


#elif (Core==4) || (Core==5)

	.text:_c_int00               > __VECS ALIGN(0x400)
    .vects:    {. = align(32); } > __CORE_SPACE ALIGN(0x400)
    .csl_vect:                   > __CORE_SPACE
    .text:csl_entry:{}           > __CORE_SPACE
    .MSMC                        > __CORE_SPACE
    .text:csl_section:intc       > __CORE_SPACE
    .bss:csl_section:intc        > __CORE_SPACE
    .ddr_functions:
    {
        udma_mem_copy.obj(.text:udma_setup)
        udma_mem_copy.obj(.text:udma_create)
        udma_soc.obj(.text:Udma_initDrvHandle)
        udma_apputils.obj(.text:UDMA_Lib_Init)
        udma_ring_common.obj(.text:Udma_ringAlloc) 
        udma_flow.obj(.text:Udma_flowConfig)
        udma_ring_normal.obj(.text:Udma_ringSetCfgNormal)
        udma.obj(.text:Udma_init)
        udma.obj(.text:UdmaInitPrms_init)
        udma_flow.obj(.text:UdmaFlowPrms_init)
        udma_ring_common.obj(.text:Udma_ringCheckParams)
        udma_ring_common.obj(.text:UdmaRingPrms_init)
        udma_rm.obj(.text)
        udma_ch.obj(.text)
        udma_event.obj(.text)
        sciclient_rm.obj(.text)
        sciclient_rm_irq.obj(.text)
        sciclient.obj(.text)
        sciclient_secureproxy.obj(.text)
        tfr_proc.obj (.text:tfr_init)
        pcie_fpga.obj (.text:train_rx_ram_addr_init)
        pcie_fpga.obj (.text:train_tx_ram_addr_init)
        pcie_fpga.obj (.text:train_all_ram_addr_init)
        gpio_intr_init.obj (.text:gpio_intr_init)
        bsp_init.obj (.text:init_core_softver)
    } > __CORE_DDR_SPACE ALIGN(0x400)
    .text:Application_InitResVersion > __CORE_DDR_SPACE ALIGN(0x400)
    .text:Application_AllTasks_I > __CORE_DDR_SPACE ALIGN(0x400)
    .text:Application_Init > __CORE_DDR_SPACE ALIGN(0x400)

    .text:                       > __CORE_SPACE
    .stack:                      > __CORE_SPACE
    GROUP:                       > __CORE_SPACE
    {
        .bss:
        .neardata:
        .rodata:
    }
    .cio:                        > __CORE_SPACE
    .c66x_const:
    {
        sciclient_irq_rm.obj(.const)
     
    } load > __CORE_DDR_SPACE ALIGN(0x200000)
    .ft3_tx_cfg_prv:                > __CORE_DDR_SPACE ALIGN(128)
    .ft3_rx_cfg_prv:                > __CORE_DDR_SPACE ALIGN(128)
    .const:                      > __CORE_SPACE
    .data:                       > __CORE_SPACE
    .switch:                     > __CORE_SPACE
    .sysmem:                     > __CORE_SPACE
    .far:                        > __CORE_SPACE
    .args:                       > __CORE_SPACE
    .ppinfo:                     > __CORE_SPACE
    .ppdata:                     > __CORE_SPACE
    .ti.decompress:              > __CORE_SPACE
    .ti.handler_table:           > __CORE_SPACE
    /* COFF sections */
    .pinit:                      > __CORE_SPACE
    .cinit:                      > __CORE_SPACE

    /* EABI sections */
    .binit:                      > __CORE_SPACE
    .init_array:                 > __CORE_SPACE
    .fardata:                    > __CORE_SPACE
    .pcie0_ib1 > L2SRAM_PCIE0 

#elif (Core==6)

    boot:
    {
      boot.*<boot.obj>(.text)
    } load > C7X_DDR_SPACE ALIGN(0x100000)
    .vecs        >       __VECS ALIGN(0x100000)
    .secure_vecs >      C7X_DDR_SPACE ALIGN(0x200000)
    .text:_c_int00_secure > C7X_DDR_SPACE ALIGN(0x200000)

    .data:Hwi_Module_state > __CORE_SPACE

    .text:Hwi_dispatchC > __CORE_SPACE
    .text:Hwi_dispatchAlways > __CORE_SPACE
    .text:Hwi_dispatchCore > __CORE_SPACE

    .text       >       __CORE_SPACE
    .bss        >       __CORE_SPACE  /* Zero-initialized data */
    .data       >       __CORE_SPACE  /* Initialized data */
    .cinit      >       __CORE_SPACE  /* could be part of const */
    .init_array >       __CORE_SPACE  /* C++ initializations */
    .stack      >       __CORE_SPACE  ALIGN(0x20000) /* Needed for Nested ISR handling */
    .args       >       __CORE_SPACE
    .cio        >       __CORE_SPACE
    .const      >       __CORE_SPACE
    .switch     >       __CORE_SPACE /* For exception handling. */
    .sysmem     >       __CORE_SPACE /* heap */

    GROUP: >  __CORE_SPACE
    {
        .data.Mmu_tableArray          : type=NOINIT
        .data.Mmu_tableArraySlot      : type=NOINIT
        .data.Mmu_level1Table         : type=NOINIT
        .data.Mmu_tableArray_NS       : type=NOINIT
        .data.Mmu_tableArraySlot_NS   : type=NOINIT
        .data.Mmu_level1Table_NS      : type=NOINIT
    }
    
    .text:__TI_printfi:__TI_printfi > __CORE_DDR_SPACE
    .ddr > __CORE_DDR_SPACE
    .pcie0_ib1 > L2SRAM_PCIE_C66X0

#endif

    .ipc_data_buffer (NOINIT) : {} palign(128)	> __CORE_DDR_SPACE
    .resource_table :{ __RESOURCE_TABLE = .;}   > __CORE_RES
    .tracebuf                : {} align(1024)   > __CORE_RES_DATA

    .filebuf(NOLOAD) : {} > FW_SPACE

    /* this is used for ipc fast data */
    .ipc_fast_data_R0 (NOLOAD)   : {} > R0_FAST_DATA_SPACE  /* R0 */
    .ipc_fast_data_R1 (NOLOAD)   : {} > R1_FAST_DATA_SPACE  /* R1 */
    .ipc_fast_data_R2 (NOLOAD)   : {} > MSMC_R2_FAST_DATA  /* R2 */
    .ipc_fast_data_R3 (NOLOAD)   : {} > MSMC_R3_FAST_DATA  /* R3 */
    .ipc_fast_data_C60 (NOLOAD)  : {} > MSMC_C60_FAST_DATA  /* C60*/
    .ipc_fast_data_C61 (NOLOAD)  : {} > MSMC_C61_FAST_DATA  /* C61 */
    .ipc_fast_data_C70 (NOLOAD)  : {} > MSMC_C70_FAST_DATA  /* C70 */
    .ipc_fast_data_MCU_R1 (NOLOAD) : {} > MSMC_MCU_R1_FAST_DATA  /* MCU_R1 */

    .r0_ipc_data_drv_mem (NOLOAD)   : {} > R0_CORE_RESERVE_SPACE  /* R0 */
    .r1_ipc_data_drv_mem (NOLOAD)   : {} > R1_CORE_RESERVE_SPACE  /* R1 */
    .r2_ipc_data_drv_mem (NOLOAD)   : {} > R2_CORE_RESERVE_SPACE  /* R2 */
    .r3_ipc_data_drv_mem (NOLOAD)   : {} > R3_CORE_RESERVE_SPACE  /* R3 */
    .c60_ipc_data_drv_mem (NOLOAD)  : {} > C60_CORE_RESERVE_SPACE  /* C60 */
    .c61_ipc_data_drv_mem (NOLOAD)  : {} > C61_CORE_RESERVE_SPACE  /* C61 */
    .c70_ipc_data_drv_mem (NOLOAD)  : {} > C70_CORE_RESERVE_SPACE  /* C70 */
    .mcu1_ipc_data_drv_mem (NOLOAD) : {} > MCU1_CORE_RESERVE_SPACE  /* MCU1 */

#if (Core != 7)
    /* this is used for ipc scada data data */
    .ipc_scada_cfg  : {} > SCADA_CFG_SPACE
    .ipc_yx_data    : {} > YX_DATA_SPACE
    .ipc_yk_data    : {} > YK_DATA_SPACE
    .ipc_yc_data    : {} > YC_DATA_SPACE
    .ipc_yt_data    : {} > YT_DATA_SPACE
#endif
    /* this is used for ipc tfr data */
    .ipc_tfr_mem_R0 (NOLOAD)   > R0_TFR_SPACE   /* R0 */
    .ipc_tfr_mem_R1 (NOLOAD)   > R1_TFR_SPACE   /* R1 */
    .ipc_tfr_mem_R2 (NOLOAD)   > R2_TFR_SPACE   /* R2 */
    .ipc_tfr_mem_R3 (NOLOAD)   > R3_TFR_SPACE   /* R3 */
    .ipc_tfr_mem_C60 (NOLOAD)  > C60_TFR_SPACE   /* C60 */
    .ipc_tfr_mem_C61 (NOLOAD)  > C61_TFR_SPACE   /* C61 */
    .ipc_tfr_mem_C71 (NOLOAD)  > C71_TFR_SPACE   /* C71 */
    .ipc_tfr_mem_MCU1 (NOLOAD) > MCU1_TFR_SPACE  /* MCU1 */

    /*pcie ib*/
    .pcie0_ib > MSMC_PCIE0
    .pcie2_ib > MSMC_PCIE2
    .pcie3_ib > MSMC_PCIE3

    /* pscode */
    .watch_tables : palign(128) > MSMC_watch_tables
    .force_table : palign(128) > MSMC_force_tables
    .set_tables : palign(128) > MSMC_pscode_set_var
    .res_version : palign(128) > PSCode_RES_VER_SPACE
    .watch_data : palign(128) > MSMC_watch_data
    .psc_data : palign(128) > __CORE_DDR_SPACE
    .wave_list : palign(128) > MSMC_wave_list
    .wave_data : palign(128) > MSMC_wave_data
    .wave_data_tmp : palign(128) > MSMC_wave_data_tmp
    .wave_info_tmp : palign(128) > MSMC_wave_info_tmp

    .irigb_shm_section: palign(128) > MSMC_IRIGB
    .fpga_monitor_shm_section: palign(128) > MSMC_PCIe_MONITOR

    .init_flag > INIT_FLGA_SPACE

    .ft3_tx_cfg: palign(128)  > FT3_TX_CFG_SPACE
    .ft3_rx_cfg: palign(128)  > FT3_RX_CFG_SPACE

    .squ_mod_cfg: palign(128)  > SQU_MOD_CFG_SPACE

    /* ipc enet */
    .ipc_shm_info         : palign(128) > ETH_INFO_SPACE
    .rx_enetque           : palign(128) > ETH_RX_QUE_SPACE

    .r1_tx_r0_enetque     : palign(128) > R1_TX_R0_ETH_MEM
    .r2_tx_r0_enetque     : palign(128) > R2_TX_R0_ETH_MEM
    .r3_tx_r0_enetque     : palign(128) > R3_TX_R0_ETH_MEM
    .a72_tx_r0_enetque    : palign(128) > A72_TX_R0_ETH_MEM
    .r0_multicast_que     : palign(128) > R0_MULTICAST_ETH_MEM

    .r0_tx_r1_enetque     : palign(128) > R0_TX_R1_ETH_MEM
    .r2_tx_r1_enetque     : palign(128) > R2_TX_R1_ETH_MEM
    .r3_tx_r1_enetque     : palign(128) > R3_TX_R1_ETH_MEM
    .r1_multicast_que     : palign(128) > R1_MULTICAST_ETH_MEM

    .r0_tx_r2_enetque     : palign(128) > R0_TX_R2_ETH_MEM
    .r1_tx_r2_enetque     : palign(128) > R1_TX_R2_ETH_MEM
    .r3_tx_r2_enetque     : palign(128) > R3_TX_R2_ETH_MEM
    .r2_multicast_que     : palign(128) > R2_MULTICAST_ETH_MEM

    .r0_tx_r3_enetque     : palign(128) > R0_TX_R3_ETH_MEM
    .r1_tx_r3_enetque     : palign(128) > R1_TX_R3_ETH_MEM
    .r2_tx_r3_enetque     : palign(128) > R2_TX_R3_ETH_MEM
    .r3_multicast_que     : palign(128) > R3_MULTICAST_ETH_MEM

    /*---------------------------VERSION_SPACE ------------------------*/
    .version: palign(128)  > VERSION_SPACE


    /*---------------------------TEST FALG ------------------------*/
    .linux_test_flag: palign(128)  > LINUX_TEST_SPACE
}
