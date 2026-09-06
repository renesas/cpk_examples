## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU 上Data operation circuit(DOC)驱动程序的基本功能。
### 1.1 创建新工程
如需了解工程的详细创建及配置流程，请参考工程：perf_counter_cpknet_ra8t2_ep
### 1.2 添加DOC外设：Stack中添加“Monitoring - Data Operation Circuit（r_doc）”.
![alt text](images/doc0.png)
#### 1.2.1 r_doc 属性配置.
![alt text](images/doc1.png)

### 1.3 Debug 模式测试
#### 1.3.1 工程切换为debug模式，编译工程。
![alt text](<images/debug mode.png>)
#### 1.3.2 查看map文件，搜索_SEGGER_RTT 地址，重新编译后，该地址可能会改变。
![alt text](images/RTT.png)
#### 1.3.3 debug时，打开J-Link RTT Viewer 输入_SEGGER_RTT 地址
![alt text](images/RTT1.png)
#### 1.3.4 进入仿真界面，J-Link RTT Viewer 观察log输出。
![alt text](<images/debug result .png>)

### 1.4 Release 模式测试
#### 1.4.1 工程切换为Release模式，编译工程。
#### 1.4.2 打开PuTTY 设置对应串口，波特率2000000。
#### 1.4.3 用Renesas Flash Programmer下载release文件夹下生成的代码，PuTTY中观察log输出。
![alt text](<images/Release putty.png>)

