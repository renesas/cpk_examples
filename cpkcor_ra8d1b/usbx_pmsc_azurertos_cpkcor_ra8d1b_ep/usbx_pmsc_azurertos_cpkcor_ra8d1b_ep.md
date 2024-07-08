## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU在AzureRTOS下作为PMSC的基本功能。当代码运行时，RA8D1可以作为USB PMSC设备和主机通信。
代码流程说明如下：
上电后，MCU作为PMSC设备和PC通信，在PC端显示为USB drive，此例程验证需要一张SD卡支持。

### 1.1 创建新工程，BSP选择“CPKCOR-RA8D1B Core Board”，RTOS选择AzureRTOS。
### 1.2 Stack中添加“USB PMSC”，详细的属性设置请参考例程
### 1.3 在卡槽中插入MicroSD卡后，利用一根USB线连接芯片的JDBG和PC，另一根USB线连接芯片的JUSB和PC。
### 1.4 在e2 studio中调试代码，代码自由运行。PC端打开资源管理器，可以看到已经识别为USB Drive：
![alt text](images/Picture1-1.png)
可以在PC端的资源管理器中向该USB Drive写入数据或读出，就像操作一个普通的U盘一样。

## 2. 支持的电路板：
CPKCOR-RA8D1B

## 3. 硬件要求：
1块瑞萨 RA核心板：CPKCOR-RA8D1B

1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）。

1根USB Type A->Type C线 （支持USB 2.0即可）。

1张MicroSD卡。

## 4. 硬件连接：

MicroSD卡插入板背面的JTF卡槽中。

USB Type A->Type C或Type-C->Type C线连接CPKCOR-RA8D1B的JDBG和调试所用PC。

USB Type A->Type C连接CPKCOR-RA8D1B的JUSB和PC。