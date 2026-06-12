# STM32 平衡小车项目

基于 STM32F103C8T6 的两轮自平衡小车，使用 FreeRTOS 实现。

## 项目信息

- **硬件**: STM32F103C8T6 (20KB RAM, 64KB Flash)
- **核心功能**: 平衡行走、蓝牙控制、超声波避障
- **任务架构**: 控制任务 (5-10ms) + 系统/UI任务 (50-100ms)
- **通信**: 全局变量 + 标志位 (减少队列开销)

## 开发规则

1. **CubeMX 保护**: 只有 `main.c` 的 USER CODE 区域可修改，其他文件禁止改
2. **变量命名**: 全局变量驼峰+下划线，常量全大写，局部变量首词小写，函数驼峰+下划线
3. **编码原则**: 简单优先，精准修改，目标驱动

## 引脚分配

| 功能 | 引脚 | 外设 |
|------|------|------|
| 编码器1 A相 | PA6 | TIM3_CH1 |
| 编码器1 B相 | PA7 | TIM3_CH2 |
| 编码器2 A相 | PB6 | TIM4_CH1 |
| 编码器2 B相 | PB7 | TIM4_CH2 |
| I2C SCL | PB8 | 软件I2C |
| I2C SDA | PB9 | 软件I2C |
| 蓝牙 TX | PA9 | USART1 |
| 蓝牙 RX | PA10 | USART1 |
| 超声波 Trig | PA4 | GPIO |
| 超声波 Echo | PA5 | GPIO |
| PWMA (左) | PA8 | TIM1_CH1 |
| PWMB (右) | PA11 | TIM1_CH4 |
| AIN1/BIN1 | PB12/PB14 | GPIO |
| AIN2/BIN2 | PB13/PB15 | GPIO |

## 文件结构

### Core 目录 (只有 main.c 可改)
- `main.c` - 主程序入口
- `tim.c` - 定时器配置
- `gpio.c` - GPIO 初始化
- `usart.c` - 串口配置 (DMA)
- `stm32f1xx_it.c` - 中断服务函数

### Components 目录
- `BSP/I2C_Bus.c/h` - 软件I2C驱动
- `BSP/Moitor.c/h` - 电机控制
- `BSP/Ultrasonicwave_Communcation.c/h` - 超声波通信
- `Middlewares/MPU6050.c/h` - 陀螺仪加速度计
- `Middlewares/PID.c/h` - PID算法
- `Middlewares/OLED.c/h` - OLED显示
- `Middlewares/Ultrasonicwave_Avoid.c/h` - 超声波避障
- `Middlewares/BLE.c/h` - 蓝牙通信

### Project_Sheet 目录
- `框架/项目概述.md` - 项目文档
- `框架/引脚确定.md` - 引脚分配表
- `框架/freertos任务切分.md` - 任务规划
- `数据手册和芯片参考文档/` - 参考手册

## 参考手册位置

- `Project_Sheet/数据手册和芯片参考文档/STM32F103x8B数据手册.pdf`
- `Project_Sheet/数据手册和芯片参考文档/PS-MPU-6000A.pdf`
- `Project_Sheet/数据手册和芯片参考文档/TB6612FNG.pdf`
