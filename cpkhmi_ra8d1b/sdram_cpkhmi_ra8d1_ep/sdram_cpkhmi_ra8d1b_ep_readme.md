## 1.参考例程概述
该示例项目演示了基于瑞萨 RA8D1 SDRAM 驱动基本读写功能，通过J-Link RTT打印输出对应的结果。


### 1.1 打开工程


### 1.2 编译，下载，运行


## 2. 结果分析

### 2.1 代码中通过把 32K byte SRAM数据写到SDRAM，然后从SDRAM读取32K byte数据，最后把读回来的数据和写进去的数据作对比，如果成功，log最后会显示 SDRAM test pass


![alt text](images/sdram.jpg)


## 4. 支持的电路板：
CPKHMI-RA8D1B

## 5. 硬件要求：
1块瑞萨 RA8D1 HMI板：CPKHMI-RA8D1B

1根 Type-C USB 数据线


## 6. 硬件连接：
通过Type-C USB 数据线将 CPKHMI-RA8D1B板上的 USB 调试端口（JDBG）连接到主机 PC