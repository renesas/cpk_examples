## Working.......

## RA8的调试和烧录接口

### RA8 MCU支持的调试和烧录接口

RA8 MCU 支持以下调试接口：SWD，JTAG，跟踪调试接口（SWO单线和TPIU四线），除了调试软件，还可以通过调试接口实现MCU片内Flash的烧录，以及外部Flash烧录（例如J-Flash配合JLink使用）。

除了使用调试器进行Flash烧录，RA8 MCU本身也支持Boot模式，进入Boot模式后，RA8 MCU内置的工厂Boot程序会和上位机通信，实现Flash烧录和其他功能（如产品声明周期管理，Trustzone边界设定等）。有关工程Boot程序的详情，请参考[RA8D1工厂Boot程序应用笔记](https://www.renesas.cn/cn/zh/document/apn/renesas-boot-firmware-ra8d1-mcu-group)。

RA8 MCU支持的Boot模式主要有以下几种: 串口Boot（通过UART9的P209/P208端口），USB Boot（通过USB FS接口），SWD/JTAG Boot。其中串口Boot和USB Boot不需要额外的硬件工具，SWD/JTAG则需要对应的调试器或烧录器配合使用。

可以看到，通过SWD或JTAG接口，可以实现调试和Boot模式两种功能，我们首选SWD作为调试和烧录接口。

### 核心板上的板载调试器

CPKCOR-RA8D1B板载了JLink调试器，运行JLink固件的是瑞萨的RA4M2 MCU，全速USB接口。

基于RA4M2的板载JLink调试器可以支持以下通用功能：
- SWD调试， JTAG调试， 
- SWO跟踪
- USB转UART，可选硬件流控功能
JLink调试器还针对瑞萨的RA4/6/8 MCU特别提供Boot模式支持，包括：
- 带MD控制功能的串口Boot（复用JTAG管脚）
- SWD Boot








[返回目录](01_overview.md)             [RA8 USB 2.0 高速接口](07_usbhs.md)
