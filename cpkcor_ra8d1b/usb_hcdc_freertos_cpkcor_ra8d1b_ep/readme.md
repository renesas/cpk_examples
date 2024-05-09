**该示例工程由 瑞萨电子-王瑾 提供，2024年5月7日**

### 工程概述

- 该示例工程演示了基于瑞萨FSP的瑞萨RA MCU基于FreeRTOS USB HCDC的基本功能。

### 支持的开发板 / 演示板：

- CPKCOR-RA8D1B
   
### 硬件要求：

- 2块Renesas RA8开发板：CPKCOR-RA8D1B，1块作为Host，1块作为Device（烧录usb_pcdc_cpkcor_ra8d1b_ep代码）。

- 1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）。

- 1根USB Type-C->Type-C线。

### 硬件连接：

- 1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）连接HCDC板的JDBG和调试所用PC。

- 1根USB Type-C->Type-C线连接HCDC板的JUSB和PCDC板的JUSB。

### 硬件设置注意事项：

- 无

### 软件开发环境：
   
- FSP版本
  - FSP 5.3.0
- 集成开发环境和编译器：
  - e2studio v2024-04 + LLVM v17.0.1
  
### 第三方软件
- 无	   

**详细的样例程序配置和使用，请参考下面的文件。**

[usb_hcdc_freertos_cpkcor_ra8d1b_ep_readme](usb_hcdc_freertos_cpkcor_ra8d1b_ep_readme.md)
