## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU 上 OSPI_B 驱动程序的基本功能。

### 1.1 创建新工程，BSP选择“CPK-RA8D1B Core Board”
### 1.2 Stack中添加“OSPI Flash (r_ospi_b)”，详细的属性设置请参考例程
![alt text](images/Picture1-1.png)
### 1.3 根据QSPI Flash的数据手册，调整OSPICLK
参考该板上的QSPI Flash的数据手册，读写操作时的最大频率如下：

![alt text](images/Picture2-1.png)

建议修改OSPICLK为100MHz。

![alt text](images/Picture3-1.png)

### 1.4 具体操作：
#### 1.4.1 用户选择 opsi_b 模块的模式：SPI 1S-1S-1S 协议模式。
#### 1.4.2 读取Manufacture/Device ID信息，判断板上QSPI为AT25SF128A还是W25Q128JVPIQ
#### 1.4.3 擦除QSPI FLASH 的一个扇区。
#### 1.4.4 将数据写入 QSPI FLASH。
#### 1.4.5 从QSPI FLASH读回数据，并进行比较，确认写入成功。也可以从Memory窗口看到写入的数据。
![alt text](images/Picture4-1.png)
#### 1.4.6 因为AT25SF128A默认为Single模式，所以需要通过设置Flash内部状态寄存器允许Quad操作。而Winbond W25Q128JVPIQ默认就是Quad模式，所以这里无需特别处理。
#### 1.4.7 切换模式：SPI 1S-4S-4S 协议模式
#### 1.4.8 从QSPI FLASH读回数据，并进行比较。

## 2. 支持的电路板：
CPKCOR-RA8D1B

## 3. 硬件要求：
1块瑞萨 RA核心板：CPKCOR-RA8D1B（板上QSPI Flash为Renesas AT25SF128A或者Winbond W25Q128JVPIQ）

1根Type-C USB 数据线

## 4. 硬件连接：
通过Type-C USB 数据线将 CPKCOR-RA8D1B板上的 USB 调试端口（JDBG）连接到主机 PC。

## 5. 使用JFlash Lite对外部QSPI Flash进行操作：
运行JFlash Lite

添加需要烧录的.srec文件，可以在Log窗口看到改烧录文件中数据所在地址范围和大小。

![alt text](images/Picture5-1.png)

由于该核心板上QSPI Flash使用的的片选信号为CS1。

![alt text](images/Picture6-2.png)

所以，对应的QSPI. FLASH的地址范围为0x9000_0000~0x9FFF_FFFF。

![alt text](images/Picture7-1.png)

## 6. QSPI速度读写测试

在代码流程的最后，会提示输入确认是否需要进行QSPI的读写速度测试，通过RTTViewer界面输入1即可进行QSPI读写速度测试，如下图：

![](images/qspi_test.png)



包含以下测试项：

1. QSPI flash擦除测试
2. 读写速度测试。