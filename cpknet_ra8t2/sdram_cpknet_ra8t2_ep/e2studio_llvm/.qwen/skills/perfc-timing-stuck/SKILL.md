---
name: perfc-timing-stuck
description: 诊断 perf_counter 库中 get_system_us() 时间卡死（返回相同值）的问题——常见于 SDRAM 等性能测试中出现 "Test size is too small" 的情况
source: auto-skill
extracted_at: '2026-06-18T06:03:35.903Z'
---

# perf_counter 时间卡死诊断

## 症状
使用 `get_system_us()` 测量时间时，`time_start == time_end`，导致所有后续测试输出 "Test size is too small with this case"。

## 诊断步骤（按顺序执行）

### 1. 先检查工程编译宏——确定时间基准来源
查看 `.cproject` 或编译选项中的宏定义，**这决定了时间基准是 Systick 还是 PMU**：

```
grep -r "__PERFC_USE_PMU_PORTING__" .cproject
grep -r "__PERFC_CFG_DISABLE_DEFAULT_SYSTICK_PORTING__" .cproject
```

- **如果定义了 `__PERFC_USE_PMU_PORTING__=1`**：时间基准是 **PMU Cycle Counter**（CCNTR，32-bit 硬件计数器，`perfc_port_pmu.c`），跳转到 **PMU 专有诊断（步骤 2）**
- **如果未定义**：时间基准是 **Systick**（24-bit 递减计数器，`perfc_port_default.c`），跳转到 **Systick 专有诊断（步骤 3）**

### 2. PMU 专有诊断：32-bit 溢出 + DebugMon 竞争

#### 2.1 计算 PMU CCNTR 溢出周期
```c
// PMU CCNTR 是 32-bit 计数器，在 SystemCoreClock 频率下递增
// 溢出周期 = 0x100000000 / SystemCoreClock
```
- 600MHz：溢出周期 = 4,294,967,296 / 600,000,000 ≈ **7.16 秒**
- 1000MHz：溢出周期 = 4,294,967,296 / 1,000,000,000 ≈ **4.29 秒**

#### 2.2 分析溢出处理路径
PMU 溢出由 **Debug Monitor 异常**（`DebugMon_Handler`）处理：
```c
// perfc_port_pmu.c
void DebugMon_Handler(void) {
    perfc_port_pmu_insert_to_debug_monitor_handler();
    // 检查 PMU->OVSSET & CYCCNT_STATUS_Msk
    // 调用 perfc_port_insert_to_system_timer_insert_ovf_handler()
    //   → lTimestampBase += 0x100000000
}
```

同时 `check_systick()` → `__check_and_handle_ovf()` 也会检查并处理溢出：
```c
// perf_counter.c
if (perfc_port_is_system_timer_ovf_pending()) {  // PMU->OVSSET & CYCCNT_STATUS
    perfc_port_clear_system_timer_ovf_pending();
    perfc_port_insert_to_system_timer_insert_ovf_handler();
    lTemp = perfc_port_get_system_timer_elapsed();
}
```

#### 2.3 竞争条件分析
关键问题：**DebugMon_Handler 和 check_systick() 可能同时处理同一个溢出**（双重处理），或 **CCNTR 溢出多次但 check_systick() 只处理一次**（漏处理，因为 OVSSET 是单比特标志）。

- 如果双重处理：`lTimestampBase` 多加 0x100000000 → 单调性守卫钳位（`lTemp` 永远小于 `lOldTimestamp`）
- 如果漏处理：`lTimestampBase` 少加 0x100000000 → 同样导致钳位

#### 2.4 验证时钟与溢出时序
计算测试过程中 CCNTR 的增量，判断是否跨越溢出边界：
```
CCNTR 增量 = 测试耗时(μs) × SystemCoreClock(MHz)
```
如果 `CCNTR_起始值 + CCNTR_增量 > 0xFFFFFFFF`，则测试期间必然溢出。

### 3. Systick 专有诊断

#### 3.1 检查 `__PERFC_USE_DEDICATED_MS_AND_US__` 是否定义
- **未定义**：`get_system_us()` 使用内联简化版本，依赖 `get_system_ticks()` 的单调性守卫
- **已定义**：使用外部版本，拥有独立微秒计数器

#### 3.2 检查 `__PERFC_SYSTIMER_PRIORITY__`
- 默认为 `0`：`__PERFC_SAFE` 通过 `__disable_irq()` 禁用**全部中断**
- 非零值：仅屏蔽 Systick 中断

### 4. 跟踪单调性守卫
无论哪种时间基准，`get_system_ticks()` 中的守卫逻辑相同：
```c
if (lTemp < PERFC.Ticks.lOldTimestamp) {
    lTemp = PERFC.Ticks.lOldTimestamp;  // 时间被冻结
}
```
一旦 `lOldTimestamp` 被异常置为较大值，后续所有调用返回相同值。

## 修复方案

### 针对 PMU 模式（`__PERFC_USE_PMU_PORTING__=1`）
**方案 A（推荐）**：调整 MCU 时钟频率，改变 CCNTR 溢出发生的时序，使溢出不发生在关键测试循环中。这是已验证有效的方案（将 CPUCLK 从 600MHz 改为 1000MHz）。

**方案 B**：在 `check_systick()` 中支持多次溢出处理——当前 `OVSSET` 是单比特，无法区分 1 次还是 2 次溢出。可改为在 `__check_and_handle_ovf()` 中读取 CCNTR 的绝对变化量自行计算溢出次数。

**方案 C**：定义 `__PERFC_USE_DEDICATED_MS_AND_US__`，启用专用微秒计数器（但 PMU 模式下该宏主要影响 MS/US 独立计数器，不直接解决 CCNTR 溢出竞争）。

### 针对 Systick 模式
**方案 D**：定义 `__PERFC_USE_DEDICATED_MS_AND_US__`，启用专用微秒计数器。
**方案 E**：设置 `__PERFC_SYSTIMER_PRIORITY__` 为非零值，减少全局中断嵌套。

## 相关文件
- `.cproject` — 编译宏定义（`__PERFC_USE_PMU_PORTING__`、`__PERFC_CFG_DISABLE_DEFAULT_SYSTICK_PORTING__`）
- `src/perf_counter/perf_counter.h` — `get_system_us()` 声明、`__PERFC_SAFE`/`__IRQ_SAFE` 宏
- `src/perf_counter/perf_counter.c` — `get_system_ticks()`、`check_systick()`、`__check_and_handle_ovf()`、`perfc_port_insert_to_system_timer_insert_ovf_handler()`
- `src/perf_counter/perfc_port_pmu.c` — PMU 移植：`perfc_port_get_system_timer_elapsed()`（返回 `PMU->CCNTR`）、`DebugMon_Handler`、`perfc_port_init_system_timer()`
- `src/perf_counter/perfc_port_pmu.h` — PMU 中断屏蔽函数（`perfc_port_mask_systimer_interrupt` 使用 `__set_BASEPRI_MAX`）
- `src/perf_counter/perfc_port_default.c` — Systick 移植：`SysTick_Handler`、`perfc_port_init_system_timer`
- `src/perf_counter/perfc_port_default.h` — `__PERFC_SYSTIMER_PRIORITY__` 默认值
- `src/test/test_sdram.c` — SDRAM 性能测试代码