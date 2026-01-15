该示例工程由 瑞萨电子-Heaven 提供，2025年10月13日

# 工程概述

该示例工程演示了基于瑞萨 FSP 的 RA8T2 MCU 的 EtherCAT 功能，实现简单的 IO 应用。

如果您没有同步代码库及版本控制的需求，也可以直接下载本目录下的ZIP压缩包，其中包含了文档和代码。

# 支持的开发板/演示板
CPKEXP-RA8T2

# 硬件要求

- 1 块 Renesas RA8 开发板：CPKEXP -RA8T2
- 1 根 USB Type A->Type C 或 Type-C->Type-C 线（支持 Type-C 2.0 即可）
- 1 根网线
- 1 台安装了 Twincat 的工控机

# 硬件连接

- 通过 USB Type-C 线连接调试电脑和 CPKEXP -RA8T2 板上的 USB 调试端口
- 网线连接开发板的 ETH0 和 Twincat 工控机

![hardware_connect](images/hardware_connect.png)


# 软件开发环境

- FSP 版本：FSP 6.1.0
- 集成开发环境和编译器：e2studio 2025-07 + GCC 13.2.1

# 第三方软件
无

#### 示例工程详细的配置和使用方法，请参考下面的说明文件
[ecat_io_cpknet_ra8t2_cpu0_ep](ecat_io_cpknet_ra8t2_cpu0_ep.md)


