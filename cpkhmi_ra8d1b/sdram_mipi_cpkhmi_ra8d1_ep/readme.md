该示例工程由 瑞萨电子-黄国爵 提供，2024年5月23日

- 2025年10月28日更新至 FSP6.2.0

#### 工程概述:
* 该示例工程演示了基于CPKHMI-RA8D1B的 MIPI 驱动以及SDRAM测试，支持 10.1inch 和 7inch 触摸屏，RGB565。

#### 支持的开发板 / 演示板：
* CPKHMI-RA8D1B

#### 硬件要求：
* 1块 Renesas RA8开发板：CPKHMI-RA8D1B
* 1根 USB Type A->Type C 或 Type-C->Type C 线 （支持 Type-C 2.0 即可）
* 1块 10.1inch 屏，型号为：STL10.1-32-331-A，驱动 IC 为：Ilitek-ILI9881C。或者1块7inch屏，型号为：STL7.0-60-132-K，驱动IC为：EK79007AD2

#### 硬件连接：
* 通过 USB Type-C 线连接调试主机和 CPKCOR-RA8D1B 板上的 USB 调试端口。

#### 硬件设置注意事项：
* 无

#### 软件开发环境：
* FSP版本
  * FSP 6.2.0
* 集成开发环境和编译器：
  * e2studio v2025-10 + 13.2.1.arm-13-7 或者
  * e2studio v2025-10 + LLVM for ARM 18.1.3

#### 第三方软件
* 无

#### 操作步骤：
* 打开工程
* 注意board_sdram.c中的宏：BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET 和 BSP_PRV_SDRAM_BUS_WIDTH，该板子分别对应的是 8 和 1，
  如果需要测试其他SDRAM，根据spec修改这两个宏
* 工程默认使用10.1inch屏，若要测试7inch屏，需要修改配置参数，具体修改步骤，请看详细的样例程序配置和使用文档
* 编译，烧录


### 详细的样例程序配置和使用，请参考下面的文件。
[sdram_mipi_cpkhmi_ra8d1b_ep_readme](sdram_mipi_cpkhmi_ra8d1b_ep_readme.md)

