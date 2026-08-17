**该示例工程由 李昌壕 提供，2026年6月25日**

### 工程概述

- 该示例工程演示了基于瑞萨 FSP ，用RA8P1 MCU 驱动 EK79007 屏幕显示彩条，使用 MIPI 接口。
- 本目录下也存放了已经编译好的程序镜像文件（hex/srec/mot等格式），可以直接烧录到开发板上的MCU中运行，查看演示结果。
  - 有关如何烧录编译好的镜像文件，请参考[RA8 MCU的程序烧录](../../docs/ra8_nvm_programming.adoc)。  
- 如果您没有同步代码库及版本控制的需求，也可以[直接下载样例程序的ZIP压缩包](../_ep_archive/ek79007_lcd_mipi_cpkcor_ra8p1_ep_rafsp6.4.0.zip)，其中包含了文档和代码。
- 
### 支持的开发板 / 开发套件 / 演示板：

- CPKCOR-RA8P1 核心板
   
### 硬件要求：

- 1块 Renesas RA8P1 开发板：CPKCOR-RA8P1
- 1个使用 EK79007 驱动芯片的LCD屏幕模组，分辨率 1024RGB x 600
- 1个 MIPI-DSI LCD 转接板
- 1根 USB Type A->Type C 或 Type-C->Type C 线 （支持 Type-C 2.0 即可）

### 硬件连接：

- 调试主机通过 USB Type-C 线连接 CPKOR-RA8P1 板上的 USB 调试端口JDBG。
- LCD屏幕模组通过MIPI-DSI转接板连接到CPKCOR-RA8P1 核心板上的 18Pin FPC 插座
- 
### 硬件设置注意事项：

- 样例程序的主要运行参数： CPU0 - 1GHz, CPU1 - 不使用, NPU - 不使用，ICLK 250MHz
- 该运行参数是核心板实装的 RA8P1 MCU （Tj=95 摄氏度规格）可支持的最快运行速度，如果您的目标应用会使用其他温度规格的 MCU，请进行对应调整。

### 软件开发环境：

- FSP版本
  - FSP 6.4.0
- 集成开发环境和编译器：
  - e2studio v2025-12 + LLVM v21.1.1

**详细的样例程序配置和使用，请参考下面的说明文件。**

[lcd_ek79007_mipi工程使用说明](ek79007_lcd_mipi_cpkcor_ra8p1_ep_readme.adoc)

----

### 在其他开发板上使用本样例程序

- 无