**该示例工程由 瑞萨电子-Ran Qingling 提供，2026年2月5日**

### 工程概述

- 该示例工程演示了基于瑞萨FSP的瑞萨RA MCU上ADC16H: single scan mode驱动程序的基本功能。

- 如果您没有同步代码库及版本控制的需求，也可以直接下载本目录下的ZIP压缩包，其中包含了文档和代码。
  
- 本目录下也存放了已经编译好的程序镜像文件，可以直接烧录到开发板上的MCU中，查看演示结果。
  - 有关如何烧录编译好的镜像文件，请参考[RA8 MCU的程序烧录](../../docs/ra8_nvm_programming.adoc)。

### 支持的开发板 / 演示板：

- CPKNET-RA8T2
   
### 硬件要求：

- 1块Renesas RA8开发板：CPKNET-RA8T2 

- 1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）

### 硬件连接：

- 通过 USB Type-C 线连接 CPKNET-RA8T2  板上的 USB 调试端口和调试用主机。

### 硬件设置注意事项：

- 无

### 软件开发环境：
   
- FSP版本
  - FSP 6.3.0
- 集成开发环境和编译器：
  - e2studio v2025-12 + LLVM v21.1.1

### 第三方软件
- 无 
	   

**详细的样例程序配置和使用，请参考下面的文件。**

[adc_singlescan_cpknet_ra8t2_ep_readme](adc_singlescan_cpknet_ra8t2_ep_readme.md)
