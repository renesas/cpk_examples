## 1.参考例程概述
该示例项目演示了基于瑞萨 FSP 的瑞萨 RA MCU在AzureRTOS下作为PAUD的基本功能。当代码运行时，RA8D1可以作为USB PAUD设备和PC通信。
代码流程说明如下：
上电后，MCU作为PAUD设备和主机通信，建立通信后，PC通过该PAUD设备播放一段音频，PAUD设备录制后将音频文件返回给PC，通过PC端播放两个音频（原始视频和录制后的音频）并比较播放效果。

### 1.1 创建新工程，BSP选择“CPKCOR-RA8D1B Core Board”，RTOS选择AzureRTOS。
### 1.2 Stack中添加“USB PAUD”，详细的属性设置请参考例程
### 1.3 利用一根USB线连接芯片的JDBG和PC，另一根USB线连接芯片的JUSB和PC。
### 1.4 在e2 studio中调试代码，代码自由运行。打开RTT Viewer，可以看到如下Log打印，可以看到设备初始化成功：
![alt text](images/Picture1-1.png)
### 1.5 在PC端打开设备管理器，确认有以下设备
![alt text](images/Picture1-2.png)

### 1.6 继续确认有以下设备
![alt text](images/Picture1-3.png)
### 1.7 在PC端播放一段音频（wav格式，mp3格式等），播放软件如Audacity。此时MCU会录音并将从PC收到的数据存在RAM上。
### 1.8 当播放完成后，在PC端利用Audacity进行录制。此时MCU会根据收到的来自PC的请求将数据传给PC。
### 1.9 导出Audacity录制的音频，另存到电脑上。
### 1.10 导入刚才录制的视频并播放，对比录制的视频和原始视频，会发现他们非常接近。如果录制的格式和播放的不同，则音质可能会略有差别。
## 2. 支持的电路板：
CPKCOR-RA8D1B

## 3. 硬件要求：
1块瑞萨 RA核心板：CPKCOR-RA8D1B

1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）。

1根USB Type A->Type C线 （支持USB 2.0即可）。


## 4. 硬件连接：

USB Type A->Type C或Type-C->Type C线连接CPKCOR-RA8D1B的JDBG和调试所用PC。

USB Type A->Type C连接CPKCOR-RA8D1B的JUSB和PC。