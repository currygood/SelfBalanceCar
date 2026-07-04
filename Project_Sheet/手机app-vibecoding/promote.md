# Role
资深 Android 开发工程师 (Kotlin, Jetpack Compose, 经典蓝牙 SPP)

# Task
为我的平衡车开发一个控制 App。使用 VS Code 环境。

# 业务逻辑 (核心细节)
1. 屏幕模式：固定横屏 (Landscape)。
2. 蓝牙协议：
   - 帧结构：[0xAA, Cmd, Throttle, Steering, Reserve, Checksum] (6字节)
   - 校验和：前 5 字节当做无符号 Int 累加后取最低 8 位。
   - 映射：0x00=最大反向, 0x7F=中点停止, 0xFF=最大正向。
   - PID：Kp(val*10), Ki(val*100), Kd(val*10)。
3. 连接管理：
   - 提供设备扫描列表。
   - 记住上次连接的 MAC 地址（SharedPreferences），App 启动后尝试自动连接。
   - 支持手动断开或重新选择其他设备。
4. 安全逻辑：
   - 当 App 进入后台 (`onPause`)，必须立即发送一帧动力=0x7F, 方向=0x7F 的指令，确保车子原地平衡不乱跑。
   - 摇杆设置 10% 死区。
5. 通信频率：连接成功后，每 20ms 定时循环发送当前控制帧。
6. PID 操作：UI 输入数值后，点击“发送”按钮才触发 0x04 命令帧。

# 技术栈要求
- UI: Jetpack Compose。
- 异步：Kotlin Coroutines (协程)。
- 数据流：StateFlow。
- 权限：处理 Android 12+ 的运行时蓝牙权限。

请给出完整的 `BluetoothManager` 实现，以及 `ProtocolManager` 和 `Joystick` 逻辑的代码。

先帮我生成 AndroidManifest.xml 和 build.gradle.kts 的内容，这两个文件配置错了，App 是无法运行和使用蓝牙权限的。