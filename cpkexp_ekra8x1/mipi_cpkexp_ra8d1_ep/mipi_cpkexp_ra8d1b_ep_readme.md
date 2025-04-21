## 1.参考例程概述
该示例项目演示了基于瑞萨 RA8D1 MIPI 屏驱动显示功能

### 1.1 打开工程
### 1.2 可以打开 /src 下的 mipi_dsi_ep.c, 通过show_pattern函数显示不同图案：
```
typedef enum
{
    simple = 0,
    partition = 1,
    gradient = 2
} color_pattern_t;

show_pattern(partition);

```

### 1.3 连接屏幕，如下：

![alt text](images/cam_pannel.jpg)

### 1.4 注意J5的跳帽连接，屏接口显示为 MIPI 的选择如下：

![alt text](images/connector.jpg)


### 1.5 编译，下载，运行



## 2 最后显示如下

![alt text](images/show_pattern.jpg)



## 3. 支持的电路板：
CPKCOR+扩展板

## 4. 硬件要求：
* 1块 Renesas RA8开发板：CPKCOR-RA8D1B
* 1根 USB Type A->Type C 或 Type-C->Type C 线 （支持 Type-C 2.0 即可）
* 1个 MIPI 显示屏，型号是H0233S001 V1，驱动芯片是ST7796U
* 1个 扩展板

## 5. 硬件连接：
通过Type-C USB 数据线将 CPKCOR-RA8D1B 板上的 USB 调试端口（JDBG）连接到主机 PC