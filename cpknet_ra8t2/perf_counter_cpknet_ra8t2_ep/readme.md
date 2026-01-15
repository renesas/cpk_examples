**该示例工程由 李昌壕	提供，2026年1月8日**

### 工程概述

- 该示例工程演示了基于瑞萨 FSP ，在RA8T2 MCU CPU0 （Cortex-M85）上运行CoreMark测试程序，并使用 perf_counter 进行性能测试。

如果您没有同步代码库及版本控制的需求，也可以直接下载本目录下的ZIP压缩包，其中包含了文档和代码。

### 支持的开发板 / 演示板：

- CPKHMI-RA8P1
   
### 硬件要求：

- 1 块 Renesas RA8 开发板：CPKNET-RA8T2

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
  - e2studio v2025-12 + arm-none-eabi-gcc 13.2.1.arm-13-7
  - keil mdk v5.43 + ARMCC6.23

### 第三方软件
- perf_counter - [Github](https://github.com/GorgonMeducer/perf_counter/blob/CMSIS-Pack/README.md)
- CoreMark® - 代码已经集成在perf_counter中，[官方网站](https://www.eembc.org/coremark/)
	   

**详细的样例程序配置和使用，请参考下面的说明文件。**

[perf_counter性能测试工程使用说明](perf_counter_cpknet_ra8t2_ep_readme.adoc)
