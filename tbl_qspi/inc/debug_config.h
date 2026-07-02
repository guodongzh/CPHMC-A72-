#ifndef DEBUG_CONFIG_H_
#define DEBUG_CONFIG_H_

#define PRINTF_DEBUG_RUN 1
#define DEBUG_LOG        1
#define ERR_LOG          1
#define WARN_LOG         1
#define INFO_LOG         1
#define TCP_SEV_DEBUG    0
#define OPEN_MPC_DEBUG   0
#define FTP_DEBUG        1
#define SCADA_DEBUG      1
#define FLASH_DEBUG      1
#define EEPROM_DEBUG     1
#define IPC_DEBUG        1
#define DISK_DEBUG       0
#define GPIO_DEBUG       1
#define SPI_DEBUG        1
#define UART_DEBUG       1
#define DMA_DEBUG        1
#define PCIE_DEBUG       1
#define IP_DEBUG         0
#define UDP_DEBUG        0
#define TFTP_DEBUG       0
#define CFG_DEBUG        0

#include <ti/drv/uart/UART_stdio.h>
#include "ti/osal/src/printf.h"

#if (PRINTF_DEBUG_RUN == 1)

#if defined(BUILD_MCU2_0)
#define PRT_TAG            "[R5F0] "

#elif defined(BUILD_MCU2_1)
#define PRT_TAG            "[R5F1] "

#elif defined(BUILD_MCU3_0)
#define PRT_TAG            "[R5F2] "

#elif defined(BUILD_MCU3_1)
#define PRT_TAG            "[R5F3] "

#elif defined(BUILD_C66X_1)
#define PRT_TAG             "[C60] "

#elif defined(BUILD_C66X_2)
#define PRT_TAG             "[C61] "

#elif defined(BUILD_C7X_1)
#define PRT_TAG             "[C7x] "

#elif defined(BUILD_MCU1_0)
#define PRT_TAG             "[MCU_R5F0] "

#endif


#if defined(UART_CONSOLE)
#define printf_log printf_
#else
#include <stdio.h>
#define printf_log printf
#endif

#define RUN_PRINT(x) x

#else

#define RUN_PRINT(x)

#endif

#if (DEBUG_LOG == 1)
#define DEBUG_LOG_RUN(x) RUN_PRINT(x)
#else
#define DEBUG_LOG_RUN(x)
#endif

#if (ERR_LOG == 1)
#define ERROR_LOG_RUN(x) RUN_PRINT(x)
#else
#define ERROR_LOG_RUN(x)
#endif

#if (WARN_LOG == 1)
#define WARN_LOG_RUN(x) RUN_PRINT(x)
#else
#define WARN_LOG_RUN(x)
#endif

#if (INFO_LOG == 1)
#define INFO_LOG_RUN(x) RUN_PRINT(x)
#else
#define INFO_LOG_RUN(x)
#endif

#if (TCP_SEV_DEBUG == 1)
#define NET_LOG_RUN(x) RUN_PRINT(x)
#else
#define NET_LOG_RUN(x)
#endif

#if (OPEN_MPC_DEBUG == 1)
#define OPEN_MPC_RUN(x) RUN_PRINT(x)
#else
#define OPEN_MPC_RUN(x)
#endif

#if (FTP_DEBUG == 1)
#define FTP_RUN(x) RUN_PRINT(x)
#else
#define FTP_RUN(x)
#endif

#if (SCADA_DEBUG == 1)
#define SCADA_RUN(x) RUN_PRINT(x)
#else
#define SCADA_RUN(x)
#endif

#if (FLASH_DEBUG == 1)
#define FLASH_RUN(x) RUN_PRINT(x)
#else
#define FLASH_RUN(x)
#endif

#if (EEPROM_DEBUG == 1)
#define EEPROM_RUN(x) RUN_PRINT(x)
#else
#define EEPROM_RUN(x)
#endif

#if (IPC_DEBUG == 1)
#define IPC_RUN(x) RUN_PRINT(x)
#else
#define IPC_RUN(x)
#endif

#if (DISK_DEBUG == 1)
#define DISK_RUN(x) RUN_PRINT(x)
#else
#define DISK_RUN(x)
#endif

#if (GPIO_DEBUG == 1)
#define GPIO_RUN(x) RUN_PRINT(x)
#else
#define GPIO_RUN(x)
#endif

#if (SPI_DEBUG == 1)
#define SPI_RUN(x) RUN_PRINT(x)
#else
#define SPI_RUN(x)
#endif

#if (UART_DEBUG == 1)
#define UART_RUN(x) RUN_PRINT(x)
#else
#define UART_RUN(x)
#endif

#if (DMA_DEBUG == 1)
#define DMA_RUN(x) RUN_PRINT(x)
#else
#define DMA_RUN(x)
#endif

#if (PCIE_DEBUG == 1)
#define PCIE_RUN(x) RUN_PRINT(x)
#else
#define PCIE_RUN(x)
#endif

#if (IP_DEBUG == 1)
#define IP_RUN(x) RUN_PRINT(x)
#else
#define IP_RUN(x)
#endif

#if (UDP_DEBUG == 1)
#define UDP_RUN(x) RUN_PRINT(x)
#else
#define UDP_RUN(x)
#endif


#if (TFTP_DEBUG == 1)
#define TFTP_RUN(x) RUN_PRINT(x)
#else
#define TFTP_RUN(x)
#endif

#if (CFG_DEBUG == 1)
#define CFG_RUN(x) RUN_PRINT(x)
#else
#define CFG_RUN(x)
#endif

#define Debug_logTag(fmt, ...)   DEBUG_LOG_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define Debug_log(fmt, ...)   DEBUG_LOG_RUN(printf_log(fmt, ##__VA_ARGS__))
#define Debug_logError(fmt, ...)   ERROR_LOG_RUN(printf_log(PRT_TAG "[ERROR]: %s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define Debug_logWarn(fmt, ...)   WARN_LOG_RUN(printf_log(PRT_TAG "[WARN]: %s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__))
#define Debug_logInfo(fmt, ...)    INFO_LOG_RUN(printf_log(PRT_TAG "[INFO]: " fmt, ##__VA_ARGS__))
#define Debug_logOk(fmt, ...)    INFO_LOG_RUN(printf_log(PRT_TAG "[OK]: " fmt, ##__VA_ARGS__))
#define NET_log(fmt, ...)     NET_LOG_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define FTP_log(fmt, ...)     FTP_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define SCADA_log(fmt, ...)   SCADA_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define FLASH_log(fmt, ...)   FLASH_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define EEPROM_log(fmt, ...)  EEPROM_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define IPC_log(fmt, ...)     IPC_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define DISK_log(fmt, ...)    DISK_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define GPIO_log(fmt, ...)    GPIO_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define SPI_log(fmt, ...)     SPI_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define UART_log(fmt, ...)    UART_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define DMA_log(fmt, ...)     DMA_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define PCIE_log(fmt, ...)    PCIE_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define PSCTRACE(fmt, ...)    OPEN_MPC_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define PSCode_log(fmt, ...)  OPEN_MPC_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define IP_log(fmt, ...)      IP_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define UDP_log(fmt, ...)     UDP_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define TFTP_log(fmt, ...)    TFTP_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#define CFG_log(fmt, ...)     CFG_RUN(printf_log(PRT_TAG fmt, ##__VA_ARGS__))
#endif /* DEBUG_CONFIG_H_ */
