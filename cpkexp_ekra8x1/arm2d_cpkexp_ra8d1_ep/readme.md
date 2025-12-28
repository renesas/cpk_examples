该示例工程由 瑞萨电子-黄国爵 提供，2024年10月10日

- 2025年10月27日更新至 FSP6.2.0

#### 工程概述:
* 该示例工程演示了基于 CPKCOR+扩展板ARM2D benchmark工程。屏的驱动IC为ST7796U,像素大小为222x480。

#### 支持的开发板 / 演示板：
* CPKCOR+扩展板

#### 硬件要求：
* 1块 Renesas RA8开发板：CPKCOR-RA8D1B
* 1根 USB Type A->Type C 或 Type-C->Type C 线 （支持 Type-C 2.0 即可）
* 1个 MIPI 显示屏，型号是H0233S001 V1，驱动芯片是ST7796U
* 1个 扩展板

#### 硬件连接：
* 通过 USB Type-C 线连接调试主机和 CPKCOR-RA8D1B 板上的 USB 调试端口

#### 硬件设置注意事项：
* 扩展板J5的跳线选择，J5选择屏的工作模式

#### 软件开发环境：
* FSP版本
  * FSP 6.2.0
* 集成开发环境和编译器：
  * MDK
  * e2studio v2025-10 + LLVM for ARM 18.1.3

#### 第三方软件
* 无

#### 操作步骤：
* 打开工程
* 注意 board_sdram.c 中的宏：BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET 和 BSP_PRV_SDRAM_BUS_WIDTH，该板子分别对应的是 9 和 0，如果需要适配其他SDRAM，根据 spec 修改这两个宏
* 编译，烧录


### 详细的样例程序配置和使用，请参考下面的文件。
[arm2d_cpkexp_ra8d1b_ep_readme](arm2d_cpkexp_ra8d1b_ep_readme.md)
