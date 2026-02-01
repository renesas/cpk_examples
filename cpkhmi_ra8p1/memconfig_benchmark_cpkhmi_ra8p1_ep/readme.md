**该示例工程由 李昌壕 提供，2026年1月20日**

### 工程概述

- 该示例工程基于瑞萨FSP， 演示了在RA8P1 MCU上，将代码与数据放在不同内存段时对性能的影响。

### 支持的开发板 / 演示板：

- CPKHMI-RA8P1
  
### 硬件要求：

- 1 块 Renesas RA8 开发板：CPKHMI-RA8P1

- 1 根 USB Type A->Type C 或 Type-C->Type C 线（支持Type-C 2.0 即可）

### 硬件连接：

- 通过 USB Type-C 线连接 CPKHMI-RA8P1 板上的 USB 调试端口和调试用主机。

### 硬件设置注意事项：

- 无

### 软件开发环境：

- FSP版本
  - FSP 6.3.0
- 集成开发环境和编译器：
  - e2studio v2025-12 + LLVM v21.1.1

### 第三方软件
- perf_counter - [Github](https://github.com/GorgonMeducer/perf_counter/blob/CMSIS-Pack/README.md)
- CoreMark® - 代码已经集成在perf_counter中，[官方网站](https://www.eembc.org/coremark/)
	

**详细的样例程序配置和使用，请参考下面的文件。** 

[memconfig_benchmark_cpkhmi_ra8p1_ep_readme](memconfig_benchmark_cpkhmi_ra8p1_ep_readme.adoc) 

