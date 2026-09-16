# 第二步：C 板最小 NuttX BSP

本步骤实现 `STM32F407IGH6` 的最小板级启动层。目标是先得到一个可以启动 NSH、打印串口日志、显示启动状态并读取按键的固件，再进入 DBUS、CAN 和执行器开发。

## 已实现边界

| 功能 | 当前实现 |
|---|---|
| 时钟 | 12 MHz HSE，经 PLL 输出 168 MHz SYSCLK |
| Flash | `0x08000000`，1 MiB |
| 主 SRAM | `0x20000000`，112 KiB；CCM 和额外 SRAM 暂不加入堆 |
| 调试控制台 | USART6，PG14/PG9，115200-8-N-1 |
| 遥控输入预留 | USART3 RX PC11，100000 波特；帧格式由后续 DBUS 模块设置为 8-E-2 |
| MAVLink 预留 | USART1，PA9/PB7，115200 波特 |
| 状态指示 | PH10/PH11/PH12 RGB LED，GPIO 高电平点亮 |
| 用户按键 | PA0，低电平按下 |
| CAN/PWM | 在 `board.h` 固定引脚映射，驱动注册留到后续步骤 |

`platform/dji_cboard` 只包含时钟、引脚、启动和 NuttX 设备注册。机器人构型、执行器数量、控制策略和消息类型不会进入 BSP。

## 构建

可在 Ubuntu 24.04 LTS/WSL2 中安装 `make`、`python3` 和 Arm GNU Toolchain，然后运行：

```bash
./tools/build.sh --clean
```

构建脚本校验并展开 Apache 官方 NuttX 13.0.0 发布包，把自定义 `defconfig` 写入 NuttX，执行 `olddefconfig` 和并行编译，最后复制 ELF、BIN、HEX、map、段大小及版本记录到 `artifacts/`。

Windows 可使用：

```powershell
.\tools\bootstrap.ps1
.\tests\step1_layout.ps1
.\tests\step2_bsp.ps1
.\tools\build.ps1 -Clean
```

Windows 上若没有完整的 WSL 工具链，`build.ps1` 会自动使用 `C:\msys64` 和 STM32CubeIDE 自带的 Arm GCC。首次使用先运行 `tools\setup_build_env.ps1`。

## 首次上板验收

1. 只连接 ST-Link 和调试串口，保持电机、电推杆、舵机及遥控器断开。
2. 烧录 `artifacts/nuttx.hex`，复位后确认 RGB LED 从蓝色启动状态进入绿色运行状态。
3. 串口工具选择 115200-8-N-1，确认看到 `nsh>`。
4. 执行 `uname -a`、`free`、`ps`、`ls /proc`，保存输出作为 BSP 验收记录。
5. 运行按键测试应用前，用万用表再次确认 PA0 按下接地。

Windows 连接 USB 转 TTL 后，也可以自动执行串口验收并保存日志：

```powershell
.\tools\verify_step2_serial.ps1 -Port COM3
```

如果电脑只检测到一个串口，可以省略 `-Port`。日志保存在 `artifacts/step2-serial.log`。

CAN 收发、PWM 波形、DBUS 解析与失控保护将在各自模块加入时单独验收。这样每一次硬件动作都能追溯到一个清晰的软件层。
