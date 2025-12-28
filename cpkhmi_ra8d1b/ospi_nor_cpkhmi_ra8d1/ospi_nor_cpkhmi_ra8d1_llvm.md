## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU 上 OSPI 驱动程序的基本功能。

### 1.1 创建新工程，BSP选择“CPKHMI-RA8D1B + Octa-Flash”
![alt text](image/Picture1.png)
### 1.2 Stack中添加“OSPI Flash (r_ospi_b)”，详细的属性设置请参考例程
![alt text](image/Picture2.png)
### 1.3 根据QSPI Flash的数据手册，调整OSPICLK
参考该板上的QSPI Flash的数据手册，本例程读写操作时的频率可以设置成100MHz：
![alt text](image/Picture3.png)

### 1.4 具体操作：
#### 1.4.1 用户选择 opsi_b 模块的模式：SPI 1S-1S-1S 协议模式。
#### 1.4.2 擦除QSPI FLASH 的一个扇区。
#### 1.4.3 将数据写入 QSPI FLASH。
#### 1.4.4 从QSPI FLASH读回数据，并进行比较，确认写入成功。也可以从Memory窗口看到写入的数据。
![alt text](image/Picture4.png)
#### 1.4.5 代码说明
![alt text](image/Picture7.png)
![alt text](image/Picture8.png)
OSPI接口初始化完成后，默认是单线（1s-1s-1s）协议，需要通过如下对Flash的配置才可以进入八线模式：
![alt text](image/Picture9.png)
进入8线（8D-8D-8D）工作模式（ODDR）之后，需要注意的是，所有操作命令长度变成两个字节，命令码不变！
![alt text](image/Picture10.png)

同时测试用例还支持对两种模式进行芯片擦除及读写速度测试，如下图：

![](image/b1.png)

![](image/b2.png)

## 2. 支持的电路板：
CPKHMI-RA8D1B

## 3. 硬件要求：
1块瑞萨 RA核心板：CPKHMI-RA8D1B

1根Type-C USB 数据线

## 4. 硬件连接：
通过Type-C USB 数据线将 CPKHMI-RA8D1B板上的 USB 调试端口（JDBG）连接到主机 PC。

由于该核心板上QSPI Flash使用的的片选信号为CS1。

![alt text](image/Picture5.png)

所以，对应的QSPI. FLASH的地址范围为0x9000_0000~0x9FFF_FFFF。

![alt text](image/Picture6.png)