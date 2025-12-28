该示例工程由 瑞萨电子-王瑾 提供，2024年09月13日

- 2025年10月27日更新至 FSP6.2.0

### 工程概述

该示例工程演示了基于瑞萨 FSP的瑞萨RA MCU baremetal下作为PCDC （USB 2.0 Full-Speed）的基本功能。

### 支持的开发板 / 演示板：

CPKCOR-RA8D1B和扩展板

### 硬件要求：

1块Renesas RA8开发板：CPKCOR-RA8D1B和扩展板。

1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）。

1根USB Type A->Type C线 （支持USB 2.0即可）。


### 硬件连接：

USB Type A->Type C或Type-C->Type C线连接CPKCOR-RA8D1B的JDBG和调试所用PC。

USB Type A->Type C连接CPKCOR-RA8D1B扩展板的USB1和PC。


### 硬件设置注意事项：

无

### 软件开发环境：

* FSP版本
  * FSP 6.2.0
* 集成开发环境和编译器：
  * e2studio v2025-10 + LLVM v18.1.3
* 第三方软件
  * 无 
	  

### 详细的样例程序配置和使用，请参考下面的文件。

[usb_pcdc_baremetal_cpkcor_ra8d1_expansion_ep](usb_pcdc_baremetal_cpkcor_ra8d1_expansion_ep.md)
