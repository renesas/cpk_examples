**该示例工程由 瑞萨电子-黄国爵 提供，2024年5月7日**

### 工程概述:
- 该示例工程演示了基于瑞萨FSP的RA8 MCU连接SDRAM的读写测试，SDRAM型号为W9825G6KH-6I。

### 支持的开发板 / 演示板：
- CPKCOR-RA8D1B

### 硬件要求：
- 1块Renesas RA8开发板：CPKCOR-RA8D1B
- 1根USB Type A->Type C或Type-C->Type C线 （支持Type-C 2.0即可）

### 硬件连接：
- 通过 USB Type-C 线连接调试主机和 CPKCOR-RA8D1B 板上的 USB 调试端口。

### 硬件设置注意事项：
- 无

### 软件开发环境：
- FSP版本
  - FSP 5.3.0
- 集成开发环境和编译器：
  - e2studio v2024-04 + LLVM v17.0.1

#### 第三方软件
- 无

#### 操作步骤：

1.  打开工程
2. 注意board_sdram.c中的宏：BSP_PRV_SDRAM_SDADR_ROW_ADDR_OFFSET 和 BSP_PRV_SDRAM_BUS_WIDTH，该板子分别对应的是 8 和 0，  如果需要测试其他SDRAM，根据spec修改这两个宏
3. 编译，烧录
4. 打开串口助手，设置波特率为115200，8bit 位宽，1bit停止位
5. 看输出结果：对SDRAM起始地址 0x68000000的4K空间进行读写测试，32bit位宽访问，所以总的大小是16Kbyte 
   ```
   SDRAM read/write test start!  
   SDRAM write 4096 bytes data finished!  
   SDRAM read 4096 bytes data finished!  
   SDRAM read back data:  
   xxxxxx  
   ...  
   xxxxxx  
   SDRAM test pass!  
   ```

**详细的样例程序配置和使用，请参考下面的文件。**
[sdram_cpkcor_ra8d1b_ep_readme](sdram_cpkcor_ra8d1b_ep_readme.md)

