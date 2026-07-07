# 平衡小车 (Self-Balancing Car)

基于 **STM32F103C8T6** + **FreeRTOS** 的两轮自平衡小车，支持蓝牙遥控、PID 自稳控制与超声波简单避障。

## 硬件平台

| 模块 | 型号 | 接口 |
|------|------|------|
| 主控 | STM32F103C8T6 | - |
| IMU | MPU6050 (6 轴) | I2C |
| 电机驱动 | TB6612 | PWM |
| 编码器 | 正交编码器 ×2 | TIM 编码器模式 |
| 显示屏 | 0.96 寸 OLED | I2C |
| 超声波 | HC-SR04 | GPIO |
| 蓝牙 | HC-04 | USART1 (DMA) |

## 软件架构

- **RTOS**：FreeRTOS v202212.00
- **双任务模型**：
  - `Motion_Control_Task`（最高优先级，5ms 周期）：姿态解算 + 三环 PID（直立环/速度环/转向环）→ PWM 输出
  - `System_UI_Task`（普通优先级，50ms 周期）：超声波避障、OLED 刷新、电池监测
- **代码分层**：BSP 驱动层 → 算法中间件层（PID/MPU6050/OLED）→ RTOS 任务应用层

## 蓝牙控制协议

固定 6 字节帧格式：`帧头(0xAA) + 命令字 + 动力值 + 方向值 + 保留 + 校验和`

支持命令：前进/后退/转向、急停、IMU 校准、PID 参数在线调节、PID 启停等。

> 详见 [BLE蓝牙控制命令说明](Project_Sheet/项目文档/BLE蓝牙控制命令说明.md)

## 手机 App
基于 **Kotlin** + **Jetpack Compose** + **Material 3** 开发的 Android 蓝牙遥控 App，通过经典蓝牙 SPP 协议与 HC-04 模块通信，实现对平衡小车的远程操控与 PID 参数在线调节。

### 技术栈

- **语言**：Kotlin
- **UI 框架**：Jetpack Compose + Material 3
- **通信**：经典蓝牙 SPP（Serial Port Profile）
- **状态管理**：StateFlow 响应式数据流
- **导航**：Jetpack Navigation Compose
- **多语言**：支持中文 / English 切换

### 主要功能

1. **双独立摇杆操控**
   - 左侧垂直摇杆控制油门（前进/后退）
   - 右侧水平摇杆控制转向（左转/右转）
   - 内置 10% 死区过滤，松手自动归零
2. **蓝牙设备管理**
   - 一键连接已配对设备
   - 蓝牙扫描发现新设备
   - 自动记住上次连接设备，启动自动重连
3. **PID 参数在线调节**
   - 支持直立环、速度环、转向环三环独立调节
   - 实时读取/写入 Kp、Ki、Kd 参数
   - 参数值通过 6 字节协议帧发送到小车
4. **PID 启停控制**
   - 设置页一键启用/停用 PID 控制器
5. **后台自动发帧**
   - 50ms 周期持续发送控制指令
   - 摇杆空闲超过 200ms 自动发送急停帧
   - 页面切后台时自动发送急停，安全保护

### 项目结构

```
app/src/main/java/com/example/balancer/
├── MainActivity.kt          # 主 Activity，权限处理、导航、后台发送循环
├── BluetoothManager.kt      # 蓝牙连接管理、设备扫描、帧读写
├── ProtocolManager.kt       # 协议帧打包（6 字节帧 + 校验和）
├── ComposeUI.kt             # 主界面、连接页、PID 调节页 UI
├── SettingsScreen.kt        # 设置页（语言切换、PID 启停）
├── AxisJoystick.kt          # 单轴摇杆状态管理（死区 + StateFlow）
└── Joystick.kt              # 双轴摇杆状态管理
```
### 截图
<img width="2400" height="1080" alt="d6b6e67c7dcd8c5679d52ee99f741360" src="https://github.com/user-attachments/assets/4a6d2ef2-55de-456c-af96-433e3a98d060" />
<img width="2400" height="1080" alt="5e57cb15c77dafa738c43c05c406fa84" src="https://github.com/user-attachments/assets/b9e98575-c5c0-4fea-ab8a-8209621532e3" />
<img width="2400" height="1080" alt="f88fd32acd0adc876db96a1112d5cfef" src="https://github.com/user-attachments/assets/657bbf8e-2372-409f-96f8-f83d74e8cc50" />
<img width="2400" height="1080" alt="68a6f95b9cc7aca00c65379c25a5ffd8" src="https://github.com/user-attachments/assets/cf452f91-fe13-49bb-b0a7-3a0de3bc7dc1" />


## 构建

- **STM32 固件**（CMake + MinGW GCC）：

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

- **Android App**（Gradle）：

```bash
./gradlew assembleDebug
```
APK 输出路径：`app/build/outputs/apk/debug/app-debug.apk`

## 项目结构

```
├── Core/                   # CubeMX 生成的 HAL 初始化代码
├── Drivers/                # HAL 库 & CMSIS
├── components/
│   ├── BSP/                # 板级驱动（I2C/蓝牙/超声波/编码器/DWT）
│   └── Middlewares/        # 中间件（MPU6050/PID/OLED/BLE/超声波避障/任务）
├── freertos/               # FreeRTOS 内核源码
├── Project_Sheet/          # 项目文档 & 数据手册
├── Video/                  # 演示视频
└── CMakeLists.txt
```

## 演示
<img width="4096" height="3072" alt="68581a8a75cb95a070265a1ae30b4015" src="https://github.com/user-attachments/assets/249f2f94-da08-4967-913a-1b0a393241d1" />
<img width="3072" height="4096" alt="fa920776e90d409859ce7cdc0514774b" src="https://github.com/user-attachments/assets/8dcea0fb-3bc4-4b0c-8145-9df8a9d1f8f9" />

[show.mp4](Video/show.mp4)
