## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU在Baremetal下作为PCDC的基本功能。当代码运行时，RA8D1可以作为USB PCDC设备和主机通信。
代码流程说明如下：
上电后，MCU作为PCDC设备和PC通信，在PC端打开Tera Term等串口工具，根据提示信息操作即可。

### 1.1 创建新工程，BSP选择“CPKCOR-RA8D1B Core Board”，RTOS选择Baremetal。
### 1.2 Stack中添加“USB PCDC”，详细的属性设置请参考例程
### 1.3 利用一根USB线连接芯片的JUSB和PC。
### 1.4 在e2 studio中调试代码，代码自由运行。PC端打开相应串口，会显示如下Log打印，键入“1”打印当前硬件信息，键入“2”则可以打印相关支持信息：
![alt text](images/Picture1-1.png)

## 2. 支持的电路板：
CPKCOR-RA8D1B

## 3. 硬件要求：
1块瑞萨 RA核心板：CPKCOR-RA8D1B

1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）。

1根USB Type A->Type C线 （支持USB 2.0即可）。


## 4. 硬件连接：

USB Type A->Type C或Type-C->Type C线连接CPKCOR-RA8D1B的JDBG和调试所用PC。

USB Type A->Type C连接CPKCOR-RA8D1B的JUSB和PC。