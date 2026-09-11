# A72_BareMetal

J721E/DRA829 Cortex-A72_0 的 CCS managed No-OS 工程。A72 业务程序不使用
Linux、FreeRTOS 或完整 C 运行库，驱动统一使用 TI PDK No-OS OSAL。固化链按 TI 官方 J721E GP
`combined_appimage` 方式组织：TI BL31（ATF）先在 EL3 启动，加载 TI BL32
（OP-TEE），最后把本工程作为 BL33 交给 EL1 运行。

工程使用 TI CSL EL1 异常向量、No-OS `HwiP`/`SemaphoreP`、自建 EL1 MMU
页表、I/D Cache、BSS 清零、TI GICv3 中断和 ARM Generic Timer 心跳。启动代码仍兼容
JTAG 直接从 EL3/EL2/EL1 进入，但正式固化启动以 ATF -> BL33/EL1 为准。

TI `ti.csl.init.aa72fg` 中的异常向量和 IRQ dispatcher 可直接复用，工程已经
直接使用；其完整 `Entry/start_boot` 不直接替换本工程入口，因为该入口会在
EL1 再次写 `CPUECTLR_EL1` 并采用 PDK 默认内存布局，与当前 ATF 交接权限和
`0x80080000` BL33/页表布局不兼容。因此这里只保留必要的板级启动衔接，
GIC、异常向量和驱动 OSAL 均交给 TI 官方实现。

## 导入与编译

1. CCS 选择 File -> Import -> Code Composer Studio -> CCS Projects。
2. Search-directory 选择
   `E:\work\1.Project\CPHMC_A72\A72_BareMetal`。
3. 勾选发现的 `A72_BareMetal`，点击 Finish。
4. 右键工程，选择 Build Project。

构建成功后，Debug（或 Release）目录内会产生：

- `A72_BareMetal.elf`：CCS/JTAG 加载文件；
- `A72_BareMetal.ddr.bin`：完整 BL33 DDR 内容的诊断二进制；
- `A72_BareMetal.rprc`：裸机 BL33 的 TI RPRC 中间文件；
- `bl31.rprc`、`bl32.rprc`：由 PDK 官方 GP 固件转换的中间文件；
- `A72_BareMetal.appimage`：包含 OP-TEE、裸机 BL33、ATF 的完整固化镜像；
- `atf_optee_spl.appimage.hs_fs`：供现有升级表使用的同内容兼容文件名；
- `A72_BareMetal.map`：链接映射。

Console 最后出现 `A72 No-OS post-build verification: PASS` 才算完整成功。
后处理会验证 ELF 入口、各加载段、RPRC、组合镜像内三个 core/entry 记录，
并验证最终文件小于现有 1 MiB OSPI 分区。

## 固定地址和启动关系

- A72 硬件启动入口：`0x70000000`，TI BL31/ATF，运行于 EL3；
- TI OP-TEE BL32：`0x9e800000`，在组合镜像中为 `load_only`；
- 裸机 BL33 入口：`0x80080000`，由 ATF 切到 EL1 后进入；
- BL33 DDR 保留区：`0x80080000..0x81ffffff`，包含入口、异常向量、
  代码/数据、页表和栈；
- Console：WKUP_UART0，`0x42300000`；
- AppImage：J721E device ID 55；ATF 为 `mpu1_0/core 0`，OP-TEE 和
  BL33 为 `load_only/core 31`；
- 现有 TBL 的 OSPI A72 镜像槽：偏移 `0x180000`，最大 1 MiB。

`0x70000000..0x7001ffff` 保留给 ATF，本工程不再占用 MSMC。你的既有
`0x70020000` 起始区域继续留给 R5F/其他核，不受 A72 裸机工程影响。BL33
的异常向量、代码、只读数据、数据、BSS、页表和栈全部放在规划给 A72 的
DDR 区域。DDR 前 `0x80000000..0x8007ffff` 保持空出，符合 TI 官方
SPL/BL33 使用 `0x80080000` 的习惯。

## OSPI 固化启动

保留 ROM、SBL、TIFS 和 TBL。将构建生成的
`Debug\atf_optee_spl.appimage.hs_fs` 烧写到 OSPI 偏移 `0x180000`，替换
原来的 A72 Linux 组合镜像。该兼容文件名沿用旧升级表，但内容不含 U-Boot
SPL、U-Boot 或 Linux；实际内容依次为：

1. TI 官方 GP `bl32.bin`（OP-TEE，load-only）；
2. `A72_BareMetal`（BL33，load-only）；
3. TI 官方 GP `bl31.bin`（ATF，A72_0 的真正启动项）。

正常模式 0 下，TBL 解析组合 AppImage，把三个 RPRC 分别复制到目标地址，
只请求启动 core 0 的 ATF。ATF 完成 EL3 初始化后进入 OP-TEE，再把控制权
交给 `0x80080000` 的裸机 BL33/EL1。模式 3 仍严格保留为调试路径：加载
其他核的 debug/NULL 组合镜像，不从 OSPI 加载或启动 A72；它只通过 TI
Sciclient PM 接口打开 A72 模块和功能时钟，供 CCS/JTAG 单独加载
`A72_BareMetal.elf`，避免连接时报 `Error -2081`。

不要把诊断 `.bin` 烧进 OSPI；完整固化文件只有 `.appimage`（或它的
`.hs_fs` 兼容副本）。GP 器件这里使用 PDK 的非 `hs` 目录固件；扩展名
`.hs_fs` 只是现有升级表文件名，不代表镜像被 HS 签名。

## 验证

### 固化验证

1. 先备份当前 OSPI 的 `0x180000..0x27ffff`。
2. 擦除该 1 MiB 槽并写入
   `Debug\atf_optee_spl.appimage.hs_fs`。
3. 冷启动，在 TBL 中选正常模式 0；模式 3 不会固化启动 A72。
4. WKUP_UART0 应显示 A72 banner，并持续输出 heartbeat。

关键输出应为：

- `CurrentEL = 1`；
- `MMU/Cache = PASS`；
- `BSS test = PASS`；
- `Timer test = PASS`；
- heartbeat 每约 1 秒递增。

当前构建的 `TTBR0_EL1` 应为 `0x80082000`（`__mmu_l1_table`），不应再
期待旧版的 `0x70001000`。如果代码段尺寸变化导致页表顺延，以 map 文件中
的 `__mmu_l1_table` 符号为准。

### JTAG 单独验证

1. 先让现有 SBL/TBL 完成 TIFS、时钟、电源域和 DDR 初始化，建议选择
   “其他核正常启动、A72 跳过”的调试模式。
2. 连接并 halt `CortexA72_0`。
3. Load Program 选择 `Debug\A72_BareMetal.elf`，不是 `.map`。
4. 确认 `Entry`（兼容符号 `_start`）为 `0x80080000` 后 Resume。

JTAG 直接加载 ELF 是独立调试路径，不会加载 OP-TEE/ATF；本工程启动汇编会
根据当前异常级做兼容处理。正式产品路径仍应使用上述 TI 组合镜像，让 ATF
负责 EL3 并把裸机程序作为 BL33 交接到 EL1。

当前 MMU 使用恒等映射：`0x70000000..0xffffffff` 的 MSMC/DDR 规划为
Normal Write-back Cacheable、Outer-shareable，外设空间为
Device-nGnRnE、Execute-never。GICv3 Distributor、Redistributor 和 EL1
system-register interface 已完成基础初始化，但 DAIF 的 IRQ/FIQ 仍保持屏蔽，
当前心跳使用轮询。

如需从 U-Boot 的 MAIN_UART0 做 RAM 测试，可在工程属性的 GNU Compiler
Predefined Symbols 中把 `A72_CONSOLE_UART_BASE=0x42300000UL` 改为
`A72_CONSOLE_UART_BASE=0x02800000UL`，重新构建后加载 ELF。
