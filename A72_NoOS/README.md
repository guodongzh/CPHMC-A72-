# CPHMC J721E A72_0 No-OS bring-up

这是一个真正的 A72_0 裸机工程：不链接 Linux、FreeRTOS、PDK OSAL 或 C 运行库。它包含最小 AArch64 启动代码、异常向量、BSS 初始化、轮询 UART 和 ARM Generic Timer 心跳。

首版的目标不是一次接入全部外设，而是先证明下面这条链路可重复工作：

```text
ROM -> 自研板 SBL -> TIFS/DMSC -> R5F TBL/Sciserver -> A72_0 No-OS
```

R5F 上的 TBL/Sciserver 仍然保留。它负责自研板 DDR、时钟、电源域、资源管理和 A72 放行；“No-OS”指 A72_0 上没有任何操作系统。

## 1. 导入 CCS 12.8 工程

工程目录已经包含 CCS/Eclipse CDT 工程文件 `.project`、`.cproject` 和 `Makefile`，工程名为 `CPHMC_A72_NoOS`。不需要自己再新建 A72 空工程。

本工程必须作为 **Existing Makefile Project** 导入。原因是 CCS 12.8 能调试 J721E 的 Cortex-A72，但没有为它提供可用的 AArch64 managed-build compiler definition；真正的编译器是 PSDK RTOS 09.02 随附的 `aarch64-none-elf-gcc`。

在 CCS 中执行：

1. 打开 `File -> Import...`。
2. 选择 `General -> Existing Projects into Workspace`，点击 `Next`。
3. `Select root directory` 填入 `E:\work\1.Project\CPHMC_A72\A72_NoOS`。
4. 确认列表出现并勾选 `CPHMC_A72_NoOS`。
5. 不要勾选 `Copy projects into workspace`，点击 `Finish`。

不要选择 `Code Composer Studio -> CCS Projects`。那个入口只接收带 CCS managed compiler definition 的项目，而本项目有意使用外部 AArch64 GNU 工具链。

若 workspace 中已经有同名的旧工程，先在 Project Explorer 中将旧工程从 workspace 移除；不要勾选删除磁盘内容，然后再按上面步骤导入。

导入后，在 Project Explorer 里右键 `CPHMC_A72_NoOS`：

- `Build Project`：执行 `gmake all`，默认构建 WKUP_UART0 版本。
- `Clean Project`：执行 `gmake clean`，清理 WKUP 和 MAIN 两种输出。
- `Build Configurations` 不适用于这个 manual-makefile 工程；配置由 Make target 决定。

需要构建 MAIN_UART0 版本时，打开 `Window -> Show View -> Make -> Make Targets`，在 `CPHMC_A72_NoOS` 下新建 target `main` 并执行。也可以在工程目录命令行执行 `gmake main`。

CCS Console 出现下面两行就表示完整构建成功：

```text
Build and static verification: PASS
Build Finished. 0 errors, 0 warnings.
```

## 已与现有代码对齐的参数

| 项目 | 值 | 依据 |
|---|---:|---|
| SoC image ID | 55 | 现有 `tbl_qspi/make_image.mk` |
| A72_0 appimage core ID | 0 | PDK 09.02 `platform.mk` 中的 `SBL_CORE_ID_mpu1_0` |
| A72 MSMC SRAM 链接地址 | `0x70000000` | 现有 TBL 在启动第二阶段前清理的 7 MiB 区域 |
| A72 MSMC SRAM 上限 | `0x706fffff` | 现有 TBL 的清理范围 |
| A72 appimage Flash 偏移 | `0x180000` | `tbl_qspi/soc/j721e/boot_core_defs.h` |
| Flash 槽上限 | `< 0x100000` | 下一个现有镜像偏移为 `0x280000` |
| TBL 调试 UART | WKUP_UART0 `0x42300000` | `tbl_qspi/src/boot_app_main.c` |
| U-Boot 控制台 | MAIN_UART0 `0x02800000` | `k3-j721e-cphmc.dts` |

构建脚本会再次检查 PDK 的 A72_0 core ID，并拒绝生成超过现有 1 MiB Flash 槽的镜像。

## 2. 命令行编译（可选）

在 Windows PowerShell 中执行：

```powershell
cd E:\work\1.Project\CPHMC_A72\A72_NoOS
Set-ExecutionPolicy -Scope Process Bypass

# TBL 从 Flash 直接启动时使用；输出到 TBL 所用的 WKUP UART
.\build.ps1

# 从当前 U-Boot 做不改 Flash 的 RAM 测试时使用；输出到 U-Boot 主串口
.\build.ps1 -Console Main
```

本机脚本会自动找到已安装的 TI AArch64 GCC。换电脑时可显式指定：

```powershell
.\build.ps1 -ToolchainRoot C:\ti\ti-processor-sdk-rtos-j721e-evm-09_02_00_05\gcc-arm-9.2-2019.12-mingw-w64-i686-aarch64-none-elf
```

主要输出：

```text
build\wkup\cphmc_a72_noos_wkup.elf       CCS/JTAG 调试文件
build\wkup\cphmc_a72_noos_wkup.bin       U-Boot 裸二进制备用
build\wkup\cphmc_a72_noos_wkup.rprc      TI RPRC 中间文件
build\wkup\cphmc_a72_noos_wkup.appimage  TBL/OSPI 最终镜像

build\main\cphmc_a72_noos_main.elf       U-Boot RAM 冒烟测试文件
```

## 3. 先做不改 Flash 的 RAM 冒烟测试

推荐先走这一步。它能验证 A72 指令、链接地址、BSS、定时器和 MAIN_UART0，不会覆盖现有启动镜像。

1. 执行 `.\build.ps1 -Console Main`。
2. 把 `build\main\cphmc_a72_noos_main.elf` 放到 SD 卡 FAT 分区，或准备好 TFTP。
3. 正常启动当前 U-Boot，执行 `help bootelf`，确认命令存在。
4. 以下以 MMC 0 的第 1 分区为例；板上编号不同就先用 `mmc list` 和 `ls mmc 0:1` 确认。

```text
fatload mmc 0:1 0x90000000 cphmc_a72_noos_main.elf
bootelf -p 0x90000000
```

若通过 TFTP：

```text
dhcp 0x90000000 cphmc_a72_noos_main.elf
bootelf -p 0x90000000
```

`bootelf -p` 会按 ELF program header 把段加载到 `0x70000000` 并跳到入口。程序不会返回 U-Boot，恢复方式是复位或重新上电。

如果该 U-Boot 没有 `bootelf`，可加载 `cphmc_a72_noos_main.bin` 后执行：

```text
fatload mmc 0:1 0x90000000 cphmc_a72_noos_main.bin
cp.b 0x90000000 0x70000000 ${filesize}
dcache flush
go 0x70000000
```

在 U-Boot 使用的 MAIN_UART0 终端上应看到：

```text
CPHMC J721E Cortex-A72_0 No-OS
MPIDR_EL1  = ...
CurrentEL  = ...
CNTFRQ_EL0 = ...
UART base  = 0x0000000002800000
BSS test   = PASS
Timer test = PASS
heartbeat=1 ...
heartbeat=2 ...
```

判定标准：`BSS test` 和 `Timer test` 都为 `PASS`，并且心跳约每秒递增一次。`CurrentEL` 是 1、2 或 3 都能运行；从 U-Boot 跳转时通常不是与 TBL 直启完全相同的 EL，所以必须再做下一步。

## 4. 验证现有 TBL 直接启动

这一步会替换 Flash 中原来给 A72/Linux 使用的镜像。写入前必须保证有可用的恢复方法，例如现有下载工具、JTAG Flash Writer，或可从其他启动介质恢复。

1. 执行 `.\build.ps1`，使用 WKUP UART 版本。
2. 备份 OSPI `0x180000..0x27ffff` 的原内容。这个 1 MiB 区域后面紧邻现有 `CORE0_APPS_FLASH_ADDR=0x280000`，不要扩大擦除范围。
3. 使用团队当前给 `atf_optee_spl` 编程的同一工具，把 `build\wkup\cphmc_a72_noos_wkup.appimage` 写入 OSPI 偏移 `0x180000`。
4. 写后回读与源文件逐字节比较，再复位。

若当前 U-Boot 能访问启动 OSPI，可在确认设备树对应的启动 Flash 是 SPI bus 0、CS 0 后使用下面的方法。先执行 `sf probe 0:0` 并核对打印出的 JEDEC 型号；型号不对就停止，不要擦写。

```text
# appimage 已放入 MMC FAT 分区
fatload mmc 0:1 0x90000000 cphmc_a72_noos_wkup.appimage

# 自研代码的 TBL 从 flash_ctrl0/OSPI port 0 读取
sf probe 0:0

# 先备份现有 1 MiB A72 启动槽到 MMC
sf read 0x91000000 0x180000 0x100000
fatwrite mmc 0:1 0x91000000 a72_slot_backup.bin 0x100000

# 只更新新镜像覆盖到的扇区
sf update 0x90000000 0x180000 ${filesize}

# 回读并比较
sf read 0x91000000 0x180000 ${filesize}
cmp.b 0x90000000 0x91000000 ${filesize}
```

`cmp.b` 必须报告数据相同。复位后，在 TBL 使用的 WKUP_UART0 终端上观察。预期 TBL 日志包含：

```text
loading image from 0x180000
BootImage completed, status = 0
SBL_SlaveCoreBoot completed for Core ID#0, Entry point is 0x70000000
```

随后应出现与 RAM 测试相同的 No-OS banner、`PASS` 和持续心跳。TBL 在 A72 启动后还会打印自己的日志，两边共用 WKUP UART 时字符可能短暂交错，这不代表 A72 异常。

## 5. CCS/JTAG 调试

自研板 DDR 参数来自现有 SBL，所以最稳妥的调试顺序是：

1. 使用你们自研板已有的 J721E `.ccxml`；本工程不绑定 XDS 型号，也不使用 TI EVM 板级配置代替自研板配置。
2. 让现有 SBL/TBL 完成 TIFS、时钟、电源域和板级初始化。首版代码位于 MSMC SRAM，本身不依赖 DDR。
3. 在 CCS Debug 视图连接 `CortexA72_0`，然后 halt 该核。
4. 右键 `CortexA72_0 -> Load -> Load Program...`，选择 `build\wkup\cphmc_a72_noos_wkup.elf`；若串口接在 MAIN_UART0，则选择 `build\main\cphmc_a72_noos_main.elf`。
5. 确认 PC 为 `_start`/`0x70000000`，再 Resume。

不要只运行 TI EVM 初始化脚本后就判断自研板失败；自研板 DDR、PMIC 和 SerDes 配置应继续以你们的 SBL 为准。如果 TBL 仍可能在后台再次释放 A72，调试时也 halt TBL 所在 R5F，避免两边同时改 A72 boot vector。

## 6. 当前工程边界和下一步

当前版本故意保持 MMU、D/I Cache、GIC 中断和外设驱动未启用，所有 IRQ 都被屏蔽，并且先运行在 MSMC SRAM。这使第一阶段问题可以限定在“镜像装载、A72 放行、CPU 执行、串口和计时器”，不把 DDR 带进首轮排查。

建议按下面顺序继续扩展：

1. 增加 EL 统一策略、MMU 页表、Cache 和 barrier/cache-maintenance API。
2. 初始化 GICv3，并用 ARM Generic Timer PPI 做第一个中断闭环。
3. 定义 A72 与 R5F Sciserver 的 TISCI、资源和中断路由边界。
4. 接入 MAIN_UART 驱动，再做 UDMA memcpy/中断测试。
5. PCIe 单独完成 SerDes、时钟、电源域、地址窗和 DMA 一致性验证。
6. Ethernet 先决定使用 MCU CPSW 还是 MAIN CPSW、由哪个核拥有，再移植 Enet LLD/lwIP 或自研协议栈。

不要直接把 PDK 的 R5F 外设例程改一下 core 名就当成 A72 支持。PDK 09.02 中许多驱动示例和库没有以 `mpu1_0` 为验证目标；A72 裸机需要逐项处理地址映射、Cache 一致性、中断路由和 TISCI 资源权限。
