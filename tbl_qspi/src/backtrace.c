/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       backtrace.c
*@author     LiuRui
*@date       2025.03.22
*@brief      backtrace
*@par        History
*Date        Version   Author     Description
*2025.03.22  1.0       LiuRui     first version
******************************************************************************/
#include "backtrace.h"
#include "debug_config.h"
#include "ti/osal/DebugP.h"
#include "src/interrupt_priv.h"
#include "csl_arm_r5.h"

#ifdef FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#endif

extern hard_fault_regs_t hard_fault_regs_c;

/* Current Process Status */
extern uint32_t gCurrentProcessorState;

static inline bool disassembly_ins_is_bl_blx(uint32_t instr)
{
   /* [31:28]==cond, [27:25]==0b101,
    * if [24]== 1 instruction is BL,
    * if [24]== 0 instruction is B,
    */
   if ((((instr >> 25) & 0x7) == 0x5) && (((instr >> 24) & 0x1) == 1))
       return true;

   /*
    * The BLX instruction has a fixed encoding of instr [27:4]==0x12FFFF3,
    * with the low 4 bit being the target register and the hight 4 bits being the condition
    */
   if ((instr & 0x0FFFFFF0) == 0x012FFF30)
       return true;
   return false;
}

extern uint32_t __stack[];
extern uint32_t __STACK_END[];
extern uint32_t __start_text[];
extern uint32_t __end_text[];


uint32_t backtrace_call_stack(uint32_t *buffer, uint32_t buf_depth, uint32_t sp)
{
   uint32_t pc, lr;
   uint32_t stack_start_addr = (uint32_t)__stack;
   uint32_t text_start_addr = (uint32_t)__start_text;
   uint32_t text_end_addr = (uint32_t)__end_text;
   uint32_t depth = 0;

#if defined(FREERTOS)
   uint32_t stack_end_addr = (uint32_t)vGetCurrentTaskStackEnd();
#else
   uint32_t stack_end_addr = (uint32_t)__STACK_END;
#endif

   for (; sp < stack_end_addr; sp += sizeof(uint32_t))
   {
       /* the *sp value may be LR */
       lr = *((uint32_t *)sp);
       /* the Cortex-R using ARM instruction, so the pc must be 4 byte aligned */
       if (lr % 4 != 0)
       {
           continue;
       }
       if ((lr >= text_start_addr + sizeof(uint32_t))
           && (lr <= text_end_addr)
           && (depth < buf_depth))
       {
           /* the *sp value may be LR, so need decrease a word to PC */
           pc = lr - sizeof(uint32_t);
           /* check the the instruction is 'BL' or 'BLX' */
           disassembly_ins_is_bl_blx(*(uint32_t *)pc);
           /* store pc to buffer */
           buffer[depth++] = pc;
       }
   }

   return depth;
}


uint32_t backtrace_buff[32];
void dump_backtrace()
{
   uint32_t sp = (uint32_t)__builtin_frame_address(0);
   uint32_t lr = (uint32_t)__builtin_return_address(0);
   uint32_t pc;
here:
   pc = (uint32_t)&&here;

   Debug_log("sp=0x%08x, lr=0x%08x, pc=0x%08x\n", sp, lr, pc);
   uint32_t depth = backtrace_call_stack(backtrace_buff, 32, sp);
   for (int i = 0; i < depth; ++i)
   {
       Debug_log("pc = %08x\n", backtrace_buff[i]);
   }
}


uint32_t ulGetDataFaultStatusRegister(void)
{
   uint32_t DFSR;
   __asm volatile("MRC p15, #0, r0, c5, c0, #0\n");
   __asm volatile("MOV %0, r0" : "=r"(DFSR));
   return DFSR;
}

uint32_t ulGetDataFaultAddressRegister(void)
{
   uint32_t DFAR;
   __asm volatile("MRC p15, #0, r0, c6, c0, #0\n");
   __asm volatile("MOV %0, r0" : "=r"(DFAR));
   return DFAR;
}

uint32_t ulGetInstructionFaultStatusRegister(void)
{
   uint32_t IFSR;
   __asm volatile("MRC p15, #0, r0, c5, c0, #1\n");
   __asm volatile("MOV %0, r0" : "=r"(IFSR));
   return IFSR;
}

uint32_t ulGetInstructionFaultAddressRegister(void)
{
   uint32_t IFAR;
   __asm volatile("MRC p15, #0, r0, c6, c0, #2\n");
   __asm volatile("MOV %0, r0" : "=r"(IFAR));
   return IFAR;
}

uint32_t ulGetCPSR(void)
{
   volatile uint32_t CPSR;
   __asm volatile("MRS %0, CPSR" : "=r"(CPSR));
   return CPSR;
}

uint32_t ulGetSPSR(void)
{
   volatile uint32_t SPSR;
   __asm volatile("MRS %0, CPSR" : "=r"(SPSR));
   return SPSR;
}


void vPortDumpExceptionState()
{
   volatile uint32_t DFSR, DFAR, IFSR, IFAR, SPSR;
   DFSR = ulGetDataFaultStatusRegister();
   DFAR = ulGetDataFaultAddressRegister();
   IFSR = ulGetInstructionFaultStatusRegister();
   IFAR = ulGetInstructionFaultAddressRegister();
   SPSR = ulGetSPSR();

   DebugP_log("R0 =0x%08x R1 =0x%08x\n", hard_fault_regs_c.R0, hard_fault_regs_c.R1);
   DebugP_log("R2 =0x%08x R3 =0x%08x\n", hard_fault_regs_c.R2, hard_fault_regs_c.R3);
   DebugP_log("R4 =0x%08x R5 =0x%08x\n", hard_fault_regs_c.R4, hard_fault_regs_c.R5);
   DebugP_log("R6 =0x%08x R7 =0x%08x\n", hard_fault_regs_c.R6, hard_fault_regs_c.R7);
   DebugP_log("R8 =0x%08x R9 =0x%08x\n", hard_fault_regs_c.R8, hard_fault_regs_c.R9);
   DebugP_log("R10 =0x%08x R11 =0x%08x\n", hard_fault_regs_c.R10, hard_fault_regs_c.R11);
   DebugP_log("R12 =0x%08x SP =0x%08x\n", hard_fault_regs_c.R12, hard_fault_regs_c.SP);
   DebugP_log("LR =0x%08x PC =0x%08x\n", hard_fault_regs_c.LR, hard_fault_regs_c.PC);
   if (gCurrentProcessorState == CSL_ARM_R5_ABORT_MODE)
   {
       DebugP_log("data addr = 0x%08x, status = 0x%08x\n", DFAR, DFSR);
       uint32_t is_lpae_format = DFSR & (1 << 0);
       uint32_t is_write = (DFSR >> 11) & 0x1;
       uint32_t status = DFSR & 0x3F;

       DebugP_log("Access type: %s\n", is_write ? "Write" : "Read");
       DebugP_log("LPAE format: %s\n", is_lpae_format ? "Yes" : "No");
       DebugP_log("Fault Status Code: 0x%02X\n", status);

       switch (status) {
       case 0x01:
           DebugP_log("Fault Type: Alignment Fault\n");
           break;
       case 0x05:
       case 0x07:
           DebugP_log("Fault Type: Translation Fault\n");
           break;
       case 0x09:
       case 0x0B:
           DebugP_log("Fault Type: Domain Fault\n");
           break;
       case 0x0D:
       case 0x0F:
           DebugP_log("Fault Type: Permission Fault\n");
           break;
       case 0x03:
           DebugP_log("Fault Type: Access Flag Fault\n");
           break;
       default:
           DebugP_log("Fault Type: Unknown or Reserved (0x%02X)\n", status);
           break;
       }

       DebugP_log("DFAR address alignment: %s\n", (DFAR % 4 == 0) ? "Aligned (4-byte)" : "Unaligned!");
   }
   else
   {
       DebugP_log("ins addr =0x%08x, status = 0x%08x\n", IFAR, IFSR);
   }
}

uint32_t call_strace[32];
void dabt_exptn_handler(void *ptr)
{
   uint32_t depth = backtrace_call_stack(call_strace, 32, hard_fault_regs_c.SP);
   DebugP_log("call_strace: arm-none-eabi-addr2line.exe -e <project name>.out -fpa ");
   DebugP_log("0x%08x ", hard_fault_regs_c.PC);
   DebugP_log("0x%08x ", hard_fault_regs_c.LR);
   for (int i = 0; i < depth; ++i)
   {
       DebugP_log("0x%08x ", call_strace[i]);
   }
   DebugP_log("\n");
   /* Go into an infinite loop.*/
   volatile bool loop = BTRUE;
   while (loop)
       ;
}
