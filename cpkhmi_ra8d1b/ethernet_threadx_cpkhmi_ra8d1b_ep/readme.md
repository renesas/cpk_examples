该示例工程由 瑞萨电子-凌滔 提供，2024年7月3日

#### 工程概述:
* 该示例工程演示了基于CPKHMI-RA8D1B的Threadx Netx Duo的网络通信程序，它包括DHCPv4 client部分和TCP/UDP 服务器/客户端例子。 

#### 支持的开发板 / 演示板：
CPKHMI-RA8D1B

#### 硬件要求：
* 1块 Renesas RA8开发板：CPKHMI-RA8D1B
* 1根 USB Type A->Type C 或 Type-C->Type C 线 （支持 Type-C 2.0 即可）
* 1根 网线
* 1个 路由器

#### 硬件连接：
* 通过 USB Type-C 线连接调试主机和 CPKHMI-RA8D1B 板上的 USB 调试端口。
* 通过网线连接CPKHMI-RA8D1B的网口与路由器

#### 硬件设置注意事项：
* 无

#### 软件开发环境：
* FSP版本
  * FSP 5.3.0
* 集成开发环境和编译器：
  * e2studio v2024-04 + LLVM for ARM 17.0.1

#### 第三方软件
* 无

#### 操作步骤：
* 打开工程
* 编译，烧录
* 打开RTT-Viewer查看调试信息


### 详细的样例程序配置和使用，请参考下面的文件。
[ethernet_threadx_cpkhmi_ra8d1b_ep_readme](ethernet_threadx_cpkhmi_ra8d1b_ep_readme.md)

