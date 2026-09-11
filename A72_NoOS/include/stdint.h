#ifndef CPHMC_A72_FREESTANDING_STDINT_H
#define CPHMC_A72_FREESTANDING_STDINT_H

/*
 * Minimal fixed-width integer definitions for the AArch64 No-OS image.
 *
 * Keeping these definitions in the project makes the source indexable in a
 * CCS manual-makefile project without depending on a CCS managed compiler
 * definition.  The sizes below are fixed by the AArch64 ABI used by
 * aarch64-none-elf-gcc.
 */
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
typedef unsigned long uintptr_t;

#endif /* CPHMC_A72_FREESTANDING_STDINT_H */
