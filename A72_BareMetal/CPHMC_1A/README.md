# CPHMC-1A A72 board support

This directory follows the CPHMC C7X layout while keeping the A72 BL33 image
freestanding:

- `common/bsp/gpio` contains only CPHMC-1A pin ownership, the GPIO LLD
  configuration table, and pad-mux setup.
- `common/ti/drv/gpio` contains the TI PDK 09.02 GPIO LLD sources
  (`GPIO_drv.c`, `GPIO_v0.c`, and their headers), retaining the same
  board/driver/SoC split as the C7X project.
- `common/ti/drv/uart` contains the TI PDK 09.02 UART LLD source
  (`UART_drv.c`, `UART_v1.c`, and their headers).  The board adaptation adds
  the PDK-missing A72 `WKUP_UART0` instance.
- GPIO and UART use TI's prebuilt A72 No-OS OSAL (`ti.osal.aa72fg`), CSL
  (`ti.csl.aa72fg`), and CSL exception-vector (`ti.csl.init.aa72fg`) libraries.
  Driver critical sections, semaphores, interrupt registration, GICv3 setup,
  and IRQ dispatch therefore use the official `HwiP`/`SemaphoreP`/`Intc_*`
  implementation rather than a board-local OSAL substitute.

The first port enables only the two pins confirmed usable on CPHMC-1A:

| Board signal | Ball | MAIN GPIO | Runtime role |
| --- | --- | --- | --- |
| `SYS_BOOTMODE0` | AD20 | `GPIO0_6` | output, initialized low |
| `SYS_BOOTMODE2` | AC29 | `GPIO0_48` | output, initialized low |

Both pads are boot straps until PORz rises.  Do not call `gpio_ctrl_init()`
from reset code; call it only after the image has been entered by the normal
boot chain.  The implementation preloads both GPIO output latches low before
switching either pad to mux mode 7.

`gpio_ctrl_init()` assumes `TISCI_DEV_GPIO0` has been brought to ON state by
the R5 boot stage before BL33 is released.  The paired boot-stage update in
this change provides that prerequisite.

The two currently exposed pins are outputs, so no GPIO pin interrupt route is
configured. GPIO read/write/toggle and all critical sections still go through
the unmodified TI GPIO LLD and TI No-OS OSAL. Enabling GPIO input interrupts
later additionally requires a board-specific TISCI interrupt-router route;
that route is independent of the OSAL/GIC integration completed here.

## UART and A72 IRQ

The console remains `WKUP_UART0` at `0x42300000`, 115200-8-N-1.  The board
wrapper now uses TI's official `UART_stdioInit(0U)` helper and the standard
`UART_putc()`/`UART_getc()` APIs.  Instance 0 is intentional: it is the only
instance that matches the WKUP UART routed to this board connector; selecting
another unused number would select a different MAIN-domain UART.

The R5 boot stage must power both `TISCI_DEV_GPIO0` and
`TISCI_DEV_WKUP_UART0` before releasing BL33.

The vendored PDK driver sources are kept source-compatible with TI 09.02.
There is one local GPIO source-level compatibility fix: GPIO initializes two
validated local indices to satisfy GCC 9.2 `-Werror`. The UART stdio sources
are copied from TI PDK 09.02 without replacing the OSAL or UART driver.
