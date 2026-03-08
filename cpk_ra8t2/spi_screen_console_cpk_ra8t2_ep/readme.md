**该示例工程由 李昌壕 提供，2026年2月10日**

### 工程概述

- 该示例工程演示了基于瑞萨 FSP ，在RA8T2 MCU CPU0 （Cortex-M85）上配合 arm2d 实现终端屏幕效果，并将 C 标准输出流重定向到屏幕。

### 支持的开发板 / 演示板：

- CPK-RA8T2
  
### 硬件要求：

- 1 块 Renesas RA8 开发板套件：CPK-RA8T2（CPKNET-RA8T2 + CPKEXP-ECSMCB）

- 1 块屏幕，分辨率 222x480，型号是H0233S001 V1，驱动芯片是ST7796U

- 1 根 USB Type A->Type C 或 Type-C->Type C 线（支持Type-C 2.0 即可）

### 硬件连接：

- 调试主机通过 USB Type-C 线连接 CPKNET-RA8T2 核心板上的 USB 调试端口JDBG。

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

[spi_screen_console工程使用说明](spi_screen_console_cpk_ra8t2_ep_readme.adoc) 
