该示例工程由 瑞萨电子-凌滔 提供，2024年07月01日

#### 工程概述:
- 该示例工程演示了基于瑞萨 FSP的瑞萨RA MCU AzureRTOS下作为HUVC的基本功能。

#### 支持的开发板 / 演示板：
- CPKCOR-RA8D1B

#### 硬件要求：
- 1块Renesas RA8开发板：CPKCOR-RA8D1B
- 1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）
- 1根USB Type C->Type A(母头)线

#### 硬件连接：
- 通过 USB Type-C 线连接调试主机和 CPKCOR-RA8D1B 板上的 USB 调试端口。
- 通过 USB Type C->Type A（母头）

#### 硬件设置注意事项：
- 无

#### 软件开发环境：
- FSP版本
  - FSP 5.3.0
- 集成开发环境和编译器：
  - e2studio v2024-04 + LLVM v17.0.1

#### 第三方软件
- 无

### 详细的样例程序配置和使用，请参考下面的文件。
[usbx_huvc_azurertos_cpkcor_ra8d1b_ep](usbx_huvc_azurertos_cpkcor_ra8d1b_ep.md)