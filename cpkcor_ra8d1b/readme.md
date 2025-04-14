## CPKCOR-RA8D1B核心板

CPKCOR-RA8D1B核心板
是瑞萨电子为中国市场设计的模块化开发板，
使用瑞萨RA8D1MCU，核心板上已经搭载了RA8 MCU支持的较为有特色的外设接口和器件，可以直接使用核心板进行学习，评估和应用开发。

点击查看[CPKCOR-RA8D1B 核心板使用手册](cpkcor_ra8d1b/docs/01_overview.md) 。

本目录下是存放CPKCOR-RA8D1B核心板的样例程序，包括：

RA8x1 MCU核心功能使用样例
- [Helium Intrinsic指令样例代码](/helium_intrinsic_cpkcor_ra8d1b_ep)

片外存储功能样例
- [16位SDRAM样例程序](/sdram_cpkcor_ra8d1_ep)
- [16位SDRAM读写性能测试](/sdram_benchmark_cpkcor_ra8d1_ep)
- [QSPI NOR Flash 样例程序](/qspi_cpkcor_ra8d1b_ep)
- [TF卡 Block Media, ThreadX](/filex_block_media_sd_cpkcor_ra8d1b_ep)
- [TF卡 exFAT + ThreadX](/filex_exfat_block_media_sd_cpkcor_ra8d1b_ep)
- [TF卡 FAT + FreeRTOS](/sdhi_freertos_fat_cpkcor_ra8d1b_ep)

裸机运行的USB样例
- [USB PCDC 样例代码](/usb_pcdc_baremetal_cpkcor_ra8d1_expansion_ep)

基于FREERTOS的USB样例
- [USB HCDC 样例代码](/usb_hcdc_freertos_cpkcor_ra8d1b_ep)
- [USB HHID 样例代码](/usb_hhid_freertos_cpkcor_ra8d1b_ep)
- [USB HVND 样例代码](/usb_hvnd_freertos_cpkcor_ra8d1b_ep)
- [USB HMSC 样例代码](/usb_hmsc_freertos_cpkcor_ra8d1b_ep)
- [USB PCDC 样例代码](/usb_pcdc_freertos_cpkcor_ra8d1b_ep)
- [USB PHID 样例代码](/usb_phid_freertos_cpkcor_ra8d1b_ep)
- [USB PVND 样例代码](/usb_pvnd_freertos_cpkcor_ra8d1b_ep)
- [USB PMSC 样例代码](/usb_pmsc_freertos_cpkcor_ra8d1b_ep)
- [USB Composite PCDC+PMSC](/usb_composite_pcdc_pmsc_freertos_cpkcor_ra8d1b_ep) 

基于AzureRTOS（ThreadX）的USB样例
- [USB HMSC 样例代码](/usbx_hmsc_azurertos_cpkcor_ra8d1b_ep)
- [USB PCDC 样例代码](/usbx_pcdc_acm_azurertos_cpkcor_ra8d1b_ep)
- [USB PMSC 样例代码](/usbx_pmsc_azurertos_cpkcor_ra8d1b_ep)
- [USB PAUD 样例代码](/usbx_paud_azurertos_cpkcor_ra8d1b_ep)
- [USB HUVC 样例代码](/usbx_huvc_azurertos_cpkcor_ra8d1b_ep)

RA8x1 MCU片内外设的使用样例
- [SCI UART printf 主机通信样例代码](/printf_sci_uart_cpkcor_ra8d1b_ep)
- [ADC 和 DAC 设置样例代码](/adc_dac_cpkcor_ra8d1b_ep)
- [VEE虚拟E2PROM样例代码](/vee_flash_cpkcor_ra8d1_ep)
- [ELC样例代码](/elc_cpkcor_ra8d1_ep)
- [AGT样例代码](/agt_cpkcor_ra8d1_ep)
- [RTC实时时钟样例代码](/rtc_cpkcor_ra8d1_ep)


瑞萨会不定期更新样例程序，请务必访问本代码仓库获取最新版本的样例。











