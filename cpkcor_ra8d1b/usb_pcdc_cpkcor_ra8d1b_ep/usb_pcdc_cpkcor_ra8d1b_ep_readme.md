## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU  USB PCDC的基本功能，注意本例程是Bare-metal类型（不使用RTOS）。当代码运行时，RA8D1的USB口会在PC的设备显示器中识别为一个COM口。

### 1.1 创建新工程，BSP选择“CPK-RA8D1B Core Board”，RTOS选择Non-RTOS。
### 1.2 Stack中添加“USB PCDC (r_usb_pcdc)”，详细的属性设置请参考例程
![alt text](images/Picture1-1.png)
### 1.3 利用一根USB线（A to C）将PC和板上的JUSB连接起来。

### 1.4 调试代码，打开PC的设备管理器，在Ports (COM & LPT)分类下可以看到两个端口，JLink CDC UART Port是板载J-Link OB对应的COM口，另外一个USB Serial Device则是RA8D1的PCDC USB端口。如下图所示：
![alt text](images/Picture2-1.jpg)
### 1.5 打开PC端的串口工具，以TeraTerm为例，打开COM45（RA8D1 USB PCDC Device）：
键入‘1’，则MCU返回当前的硬件信息：
![alt text](images/Picture3-1.png)
键入‘2’，则MCU返回RA相关的网页链接和论坛Rulz链接。
![alt text](images/Picture4-1.jpg)

## 2. 支持的电路板：
CPKCOR-RA8D1B

## 3. 硬件要求：
1块瑞萨 RA核心板：CPKCOR-RA8D1B

2根Type-C USB 数据线，一根用于连接JDBG和PC，另一根用于连接JUSB和PC。

## 4. 硬件连接：
通过Type-C USB 电缆将 CPKCOR-RA8D1B板上的 USB 调试端口（JDBG）连接到主机 PC。