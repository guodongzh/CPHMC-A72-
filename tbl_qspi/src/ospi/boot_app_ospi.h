/* Protection against multiple inclusion */
#ifndef BOOT_APP_OSPI_H_
#define BOOT_APP_OSPI_H_

#ifdef __cplusplus
extern "C"
{
#endif

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

int32_t BootApp_OSPI_Init();
void BootApp_OSPI_DeInit();
int32_t BootApp_OSPI_ReadSectors(void *dstAddr, void *srcOffsetAddr, uint32_t length);
int32_t BootApp_OSPI_EraseSectors(const uint32_t *desOffsetAddr, uint32_t length);
int32_t BootApp_OSPI_WriteSectors(void *srcAddr, void *desOffsetAddr, uint32_t length);
int32_t BootApp_OSPI_ImageLate(sblEntryPoint_t *pEntry, uint32_t imageOffset);
int32_t BootApp_OSPI_StageImage(sblEntryPoint_t *pEntry, uint32_t address);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_APP_OSPI_H_ */