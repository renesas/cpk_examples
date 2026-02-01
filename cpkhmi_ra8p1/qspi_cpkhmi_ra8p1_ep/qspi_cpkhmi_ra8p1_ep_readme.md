## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU 上 OSPI_B 驱动程序的基本功能。

### 1.1 创建新工程
### 1.2 Stack中添加“OSPI Flash (r_ospi_b)”，详细的属性设置请参考例程
![alt text](images/Picture1-1.png)
### 1.3 根据 QSPI Flash 的数据手册，调整 OSPICLK
参考该板上的 QSPI Flash 的数据手册，读写操作时的最大频率如下：

![alt text](images/Picture2-1.png)

建议修改 OSPICLK 为 100MHz。

![alt text](images/Picture3-1.png)

### 1.4 具体操作：
#### 1.4.1 用户选择 opsi_b 模块的模式：SPI 1S-1S-1S 协议模式。
#### 1.4.2 擦除 QSPI FLASH 的一个扇区。
#### 1.4.3 将数据写入 QSPI FLASH。
#### 1.4.4 从 QSPI FLASH 读回数据，并进行比较，确认写入成功。也可以从 Memory 窗口看到写入的数据。
![alt text](images/Picture4-1.png)
#### 1.4.5 切换模式：SPI 1S-4S-4S 协议模式
#### 1.4.6 从 QSPI FLASH 读回数据，并进行比较。

## 2. 支持的电路板：
CPKHMI-RA8P1

## 3. 硬件要求：
1 块 Renesas RA8 开发板：CPKHMI-RA8P1 （板上 QSPI Flash 为 Winbond W25Q256JVEIQ 或者 Winbond W25Q32JVSNIM）

1 根 Type-C USB 数据线

## 4. 硬件连接：
通过 Type-C USB 数据线将 CPKHMI-RA8P1 板上的 USB 调试端口（JDBG）连接到主机 PC。

## 5. 使用 J-Flash Lite 对外部 QSPI Flash 进行操作：
运行 J-Flash Lite

添加需要烧录的 .srec 文件，可以在 Log 窗口看到改烧录文件中数据所在地址范围和大小。

![alt text](images/Picture5-1.png)

由于该核心板上 QSPI Flash 使用的的片选信号为 CS0。

![alt text](images/Picture6-2.png)

所以，对应的 QSPI. FLASH 的地址范围为 0x8000_0000~0x8FFF_FFFF。

![alt text](images/Picture7-1.png)

## 6. QSPI速度读写测试

在代码流程的最后，会提示输入确认是否需要进行 QSPI 的读写速度测试，通过 RTTViewer 界面输入 1 和 2，即可进行 QSPI 读写速度测试，如下图：

![](images/qspi_test.png)



包含以下测试项：

1. QSPI flash 擦除测试
2. 读写速度测试。