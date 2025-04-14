## CPKHMI-RA8D1B 核心板

CPKHMI-RA8D1B核心板
是瑞萨电子为中国市场设计的模块化开发板，
使用瑞萨RA8D1MCU，支持MIPI-DSI显示输出接口。

点击查看[CPKHMI-RA8D1B 核心板使用手册](../cpkhmi_ra8d1b/docs/01_overview.md) 。

本目录下是存放CPKHMI-RA8D1B核心板特有外设的的样例程序，其他CPK-RA8x1核心板通用功能的样例程序，请参考[CPKCOR-RA8D1B样例程序](cpkcor_ra8d1b/) 。

32位SDRAM相关的样例
- [SDRAM配置样例程序](../cpkhmi_ra8d1b/sdram_cpkhmi_ra8d1_ep)
- [SDRAM性能测试](../cpkhmi_ra8d1b/sdram_benchmark_cpkhmi_ra8d1_ep)
  
MIPI LCD显示相关样例
- [RA8D1B MCU的MIPI-DSI驱动样例](../cpkhmi_ra8d1b/mipi_cpkhmi_ra8d1_ep)
- [使用SDRAM作为MIPI LCD显示缓存的性能测试](../cpkhmi_ra8d1b/sdram_mipi_cpkhmi_ra8d1_ep)
- [LVGL V8样例程序](../cpkhmi_ra8d1b/lvgl_v8_cpkhmi_ra8d1_ep)
- [LVGL V9样例程序](../cpkhmi_ra8d1b/lvgl_v9_cpkhmi_ra8d1_ep)
- [FreeRTOS下运行LVGL V9样例程序](../cpkhmi_ra8d1b/lvgl_v9_freertos_cpkhmi_ra8d1_ep)

以太网样例
- [基于FreeRTOS和embedTCP的以太网样例程序](../cpkhmi_ra8d1b/ethernet_freertos_cpkhmi_ra8d1b_ep)
- [基于ThreadX和NetX的以太网样例程序](../cpkhmi_ra8d1b/ethernet_threadx_cpkhmi_ra8d1b_ep)
- [通过以太网传输USB摄像头MotionJPEG图像的样例程序](../cpkhmi_ra8d1b/mjpg_streamer_cpkhmi_ra8d1b_ep)

瑞萨会不定期更新样例程序，请务必访问本代码仓库获取最新版本的样例。
