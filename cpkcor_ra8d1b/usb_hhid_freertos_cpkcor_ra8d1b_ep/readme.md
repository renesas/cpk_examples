该示例工程由 瑞萨电子-王瑾 提供，2024年06月28日

### 工程概述

该示例工程演示了基于瑞萨 FSP的瑞萨RA MCU在FreeRTOS下作为HHID的基本功能。

### 支持的开发板 / 演示板：

CPKCOR-RA8D1B
   
### 硬件要求：

1块Renesas RA8开发板：CPKCOR-RA8D1B。

1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）。

1根USB Type-C->Type A口母座。

1个USB键盘。

### 硬件连接：

1根Type A->Type C或Type-C->Type C线连接CPKCOR-RA8D1B板的JDBG和调试所用PC。

USB接口的键盘通过一根USB Type-C->Type A口母座连接至板上的JUSB。

### 硬件设置注意事项：

无

### 软件开发环境：
   
* FSP版本
  * FSP 5.3.0
* 集成开发环境和编译器：
  * e2studio v2024-04 + LLVM v17.0.1
* 第三方软件
  * 无 
	   

### 详细的样例程序配置和使用，请参考下面的文件。

[usb_hhid_freertos_cpkcor_ra8d1b_ep](usb_hhid_freertos_cpkcor_ra8d1b_ep.md)
