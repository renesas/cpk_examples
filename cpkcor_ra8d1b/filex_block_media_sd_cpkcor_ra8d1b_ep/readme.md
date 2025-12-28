该示例工程由 瑞萨电子-王瑾 提供，2024年06月28日

- 2025年10月28日更新至 FSP6.2.0

### 工程概述

该示例工程演示了基于瑞萨 FSP的瑞萨RA MCU基于AzureRTOS SD卡的基本功能。

### 支持的开发板 / 演示板：

CPKCOR-RA8D1B

### 硬件要求：

1块Renesas RA8开发板：CPKCOR-RA8D1B。

1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）。

1张MicroSD卡。

### 硬件连接：

板背面的卡槽JTF中插入一张MicroSD卡（提前在电脑端格式化为FAT32格式）。

1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）连接CPKCOR_RA8D1B板的JDBG和调试所用PC。

### 硬件设置注意事项：

请 ${\color{red}{\text{务必在断电情况下插拔MicroSD卡}}}$。

### 软件开发环境：

* FSP版本
  * FSP 6.2.0
* 集成开发环境和编译器：
  * e2studio v2025-10 + LLVM v18.1.3
* 第三方软件
  * 无 
	  

### 详细的样例程序配置和使用，请参考下面的文件。

[filex_block_media_sd_cpkcor_ra8d1b_ep](filex_block_media_sd_cpkcor_ra8d1b_ep.md)
