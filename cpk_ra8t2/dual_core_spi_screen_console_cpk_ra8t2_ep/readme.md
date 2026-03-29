**该示例工程由 李昌壕 提供，2026年3月19日**

### 工程概述

- 该示例工程演示了基于瑞萨 FSP ，如何将已有的单核工程（CPU0）改为双核工程，并将部分代码迁移至 CPU1 运行。
- 
- 该实例工程实现了将原先运行在CPU0的LCD Console功能迁移至CPU1运行，并在CPU0上实现输出重定向到CPU1。
- 
- 如果您没有同步代码库及版本控制的需求，也可以直接下载本目录下的ZIP压缩包，其中包含了文档和代码。
  
- 本目录下也存放了已经编译好的程序镜像文件，可以直接烧录到开发板上的MCU中，查看演示结果。
  - 有关如何烧录编译好的镜像文件，请参考[RA8 MCU的程序烧录](../../docs/ra8_nvm_programming.adoc)。

### 支持的开发板 / 演示板：

- CPK-RA8T2
  
### 硬件要求：

- 1 块 Renesas RA8 开发板套件：CPK-RA8T2（CPKNET-RA8T2 + CPKEXP-ECSMCB）

- 1 块屏幕，分辨率 222x480，型号是H0233S001 V1，驱动芯片是ST7796U

- 1 根 USB Type A->Type C 或 Type-C->Type C 线（支持Type-C 2.0 即可）

### 硬件连接：

- 调试主机通过 USB Type-C 线连接 CPKNET-RA8T2 板上的 USB 调试端口JDBG。

### 硬件设置注意事项：

- 无

### 软件开发环境：

- FSP版本
  - FSP 6.3.0
- 集成开发环境和编译器：
  - e2studio v2025-12 + LLVM v21.1.1

### 第三方软件

- arm2d - [Github]([ARM-software/Arm-2D: 2D Graphic Library optimized for Cortex-M processors](https://github.com/ARM-software/Arm-2D/tree/main)) 

- perf_counter - [Github](https://github.com/GorgonMeducer/perf_counter/blob/CMSIS-Pack/README.md) 

**详细的样例程序配置和使用，请参考下面的说明文件。**

[RA8T2利用双核重定向输出到CPU1的LCD Console - 教程](dual_core_spi_screen_console_readme.adoc) 
