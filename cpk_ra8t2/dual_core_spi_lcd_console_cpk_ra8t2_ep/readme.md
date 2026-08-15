**该示例工程由 李昌壕 提供，2026年8月14日**

### 工程概述

- 该示例工程演示了基于瑞萨 FSP ，如何将已有的单核工程（CPU0）改为双核工程，并将部分代码迁移至 CPU1 运行。
- 该实例工程进而实现了将原先运行在CPU0的LCD Console功能迁移至CPU1运行，并将CPU0的程序输出重定向到CPU1上运行的LCD Console。
- 本目录下也存放了已经编译好的程序镜像文件（hex/srec/mot等格式），可以直接烧录到开发板上的MCU中运行，查看演示结果。
  - 有关如何烧录编译好的镜像文件，请参考[RA8 MCU的程序烧录](../../docs/ra8_nvm_programming.adoc)。  
- 如果您没有同步代码库及版本控制的需求，也可以[直接下载样例程序的ZIP压缩包](../_ep_archive/dual_core_spi_lcd_console_cpk_ra8t2_ep_rafsp6.4.0.zip)，其中包含了文档和代码。

### 支持的开发板 / 开发套件 / 演示板：

- CPK-RA8T2 开发套件
  - 开发套件由 CPKNET-RA8T2 核心板 + CPKEXP-ECSMCB 扩展板组合而成
   
### 硬件要求：

- 1块 Renesas RA8T2 开发套件：CPK-RA8T2 
- 1块LCD屏幕
  - 型号: H0233S001
  - 驱动芯片: ST7796U
  - 分辨率: 222RGB x 480
- 1根 USB Type A->Type C 或 Type-C->Type C 线 （支持 Type-C 2.0 即可）

### 硬件连接：

- 调试主机通过 USB Type-C 线连接 CPKNET-RA8T2 板上的 USB 调试端口JDBG。

### 硬件设置注意事项：

- 样例程序的主要运行参数： CPU0 - 600MHz, CPU1 - 不使用, ICLK 200MHz
- 该运行参数是模拟 Tj=125 摄氏度规格的 RA8T2 MCU，核心板上实装的 RA8T2 MCU 为 Tj=95 摄氏度规格的产品，可支持的最快运行速度为 1G/250M/250M。您可以根据您的目标使用环境进行调整。

### 软件开发环境：

- FSP版本
  - FSP 6.4.0
- 集成开发环境和编译器：
  - e2studio v2025-12 + LLVM v21.1.1

### 第三方软件

- arm2d - [Github]([ARM-software/Arm-2D: 2D Graphic Library optimized for Cortex-M processors](https://github.com/ARM-software/Arm-2D/tree/main)) 

- perf_counter - [Github](https://github.com/GorgonMeducer/perf_counter/blob/CMSIS-Pack/README.md) 

**详细的样例程序配置和使用，请参考下面的说明文件。**

[RA8T2利用双核重定向输出到CPU1的LCD Console - 教程](dual_core_spi_lcd_console_readme.adoc) 

----

### 在其他开发板上使用本样例程序

- 以下开发套件上，样例程序所需要使用的硬件和本开发套件配置基本一致，修改设置后即可运行：
  - [CPKCOR-RA8T2 + CPKEXP-ECSMCB 套件](../cpkcor_ra8t2_ecsmcb/)
  - [CPKHMI-RA8P1 + CPKEXP-ECSMCB 套件](../cpkhmi_ra8p1_ecsmcb/)
  - [CPKCOR-RA8P1 + CPKEXP-ECSMCB 套件](../cpkcor_ra8p1_ecsmcb/)
- 以下开发套件上，有类似或相似的硬件功能，可将本样例程序移植到这些开发套件上运行：
  - [CPK-RA8P1 开发套件](../cpk_ra8p1/)
  - [CPKHMI-RA8P1 + CPKEXP-EK8x2 套件](../cpkhmi_ra8p1_ek8x2/)
  - [CPKNET-RA8T2 + CPKEXP-EK8x2 套件](../cpknet_ra8t2_ek8x2/)
  - [CPKCOR-RA8T2 + CPKEXP-EK8x2 套件](../cpkcor_ra8t2_ek8x2/)
  - [CPKMINI-RA8P1 开发套件](../cpkmini_ra8p1/)
  - [CPKCOR-RA8P1 + CPKEXP-MINI8x2 套件](../cpkcor_ra8p1_mini8x2/)
  - [CPKNET-RA8T2 + CPKEXP-MINI8x2 套件](../cpknet_ra8t2_mini8x2/)
  - [CPKCOR-RA8T2 + CPKEXP-MINI8x2 套件](../cpkcor_ra8t2_mini8x2/)
  
