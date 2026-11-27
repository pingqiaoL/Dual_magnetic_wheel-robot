# 双磁轮机器人下位机开发与使用说明

> 项目固件版本：**v0.1.0-dev**。文档核对日期：**2026-11-23**。
> 下位机是运行 NuttX 的 DJI C 型开发板；上位机是机载 Orin；安卓软件属于遥控端。

本文作为当前下位机工程的中文入口，说明版本、功能边界、代码框架、编译、烧录和调试。依据本地工作区源码编写，不以历史文档中的规划代替实际实现。

## 1. 版本与验证状态

### 1.1 版本定义

| 项目 | 当前值 | 依据 |
| --- | --- | --- |
| 项目名称 | 双磁轮机器人 CBoard 下位机固件 | 工程 `_code/` |
| 项目固件版本 | **v0.1.0-dev** | 本文首次定义的项目开发基线，不是 NuttX 版本 |
| 硬件目标 | DJI RoboMaster Development Board Type C | `platform/dji_cboard/` |
| 主控 | STM32F407IGH6，外部晶振 12 MHz | BSP / 板级配置 |
| RTOS | **Apache NuttX 13.0.0** | `config/versions.env`、`upstream/nuttx/.version` |
| NuttX Apps | **13.0.0** | `config/versions.env` |
| NuttX 固定源码提交 | `273c77128b6698f0c95f0d7cde1d0bb803782021` | `NUTTX_COMMIT` |
| Apps 固定源码提交 | `20ffb1a3a3b590d52890ee865a28442390e5d16c` | `NUTTX_APPS_COMMIT` |
| 当前 Git HEAD | `06007ec1b9f25ad9ec43219e8c22f6890208b5ae` | 工作区含该提交之后尚未提交的模块目录与入口重构 |
| 参考工具链 | Arm GNU Toolchain 14.3.rel1；Python ≥ 3.10 | `config/versions.env` |
| 最近留存固件的实际编译器 | STM32CubeIDE Arm GCC **11.3.1** | `artifacts/build-info.txt`，与参考版本不同 |

`v0.1.0-dev` 仅定义当前项目文档版本，不修改 NuttX 上游版本、构建脚本或板上显示。当前没有单独的项目版本打印命令，`uname -a` 显示的是 NuttX 信息，不能据此判断项目固件版本。后续若需要固件内嵌版本，应另外接入构建元数据。

版本建议使用 `主版本.次版本.修订号`，开发阶段加 `-dev`：不兼容接口或参数格式变更升级主版本；增加兼容功能升级次版本；修复问题升级修订号。正式发布时同时记录项目 Git 提交、工作区是否干净、固件 SHA-256、工具链和硬件验收结果。

### 1.2 已验证到哪一步

| 层次 | 当前证据与边界 |
| --- | --- |
| 源码实现 | 已有 SBUS、RC 处理、安全状态、控制目标、分配、CAN 输出、参数与工作队列实现 |
| 历史主机测试 | [10.14 修改记录](10.14修改文档.md) 记录第 1～7 步检查及测试通过 |
| 历史目标编译 | 留存构建时间为 `2026-10-14T04:37:57Z`；本次仅编写文档，未重新构建 |
| 固件大小 | 留存 `size.txt`：text 217544、data 1856、bss 13576 字节；不是当前堆栈峰值或整片 Flash 占用 |
| 历史板上串口 | 有 `artifacts/step2-serial.log`；只代表对应旧阶段证据 |
| 当前完整板上验收 | 10.14 记录明确注明未烧录、未进行该次重构后的板上验证，不能认定当前控制链已经实机验收 |

当前工作区含未提交修改；不能仅凭 HEAD 或旧固件文件存在，就声称当前源码已编译、烧录成功。

## 2. 当前功能与职责

| 功能 | 当前实现 | 主要代码 |
| --- | --- | --- |
| 板级启动 | 时钟、内存布局、串口控制台、LED、按键、CAN1 适配 | `platform/dji_cboard/` |
| 自动启动 | ROMFS `/etc`、NSH `rcS`、编号机器人配置 | `startup/` |
| 模块生命周期 | `start / stop / status`，模块自己的扩展命令和创建入口 | `robot/common/ModuleBase.hpp` |
| SBUS 输入 | 串口接收、帧解析、丢帧/失联状态，发布原始 RC | `SbusInput`、`robot/drivers/rc/` |
| RC 归一化 | 16 路校准、反向、死区、通道/开关映射 | `RcUpdate`、`RcConfig` |
| 安全互锁 | 上电先观察 OFF，再允许 ON；手动禁止输出；发布解锁状态 | `Command`、`ArmSafety` |
| RC 失效检测 | 检查遥控新鲜度并发布故障标志 | `Failsafe` |
| 控制目标生成 | 将手动输入与失效策略组合为速度/转向目标 | `RobotControl` |
| 构型与分配 | `CA_AIRFRAME` 选择双磁轮模型；2 路动力、2 路转向的直接矩阵分配 | `ControlAllocator`、`ActuatorEffectiveness*`、`Allocation` |
| 公共输出处理 | 功能映射、使能、新鲜度、非法数值与限幅 | `MixingOutput`、`OutputInterface` |
| CAN 输出 | 4 个物理参数槽；达妙帧编码、管理帧与反馈解码 | `CanOutput`、`DamiaoProtocol` |
| 参数管理 | 类型校验、运行期修改、更新通知、A/B Flash 保存和 CRC | `robot/params/` |
| 任务调度 | 3 个独立任务 + 2 条工作队列；消息回调触发与定时兜底 | `robot/os/`、`robot/work_queue/` |
| 进程内消息 | 自有轻量 uORB 风格发布/订阅；`.msg` 生成 C++ 定义 | `robot/orb/`、`msg/` |

以下内容**尚未实现或未接入**：升降机构的接触/压力闭环、测厚就绪状态对 Orin 的发布、Orin—STM32 的 XRCE-DDS 通信、PWM 执行器硬件后端、CAN2 输出后端、IMU 与电池监控业务。

`cboard_link` 通过 USART1 与 Orin 同步测厚就绪状态。推杆控制发布 `measurementReady`，串口任务在状态变化时发送，并响应 Orin 的状态查询。测厚仪采集和摄像头处理运行在 Orin。

## 3. 软件分层与数据流

### 3.1 控制链路

```text
MK32 接收机 SBUS
  → USART3 /dev/ttyS2 → SbusInput → input_rc
  → RcUpdate
      ├─ rc_channels                         校准后的通道
      ├─ manual_control → Failsafe → failsafe_status ─┐
      │                 └─────────────────────────────┤
      │                                               ↓
      ├─ manual_control ────────────────────────→ RobotControl
      │                                               ↓ robot_control_setpoint
      │                                         ControlAllocator
      │                                  ActuatorEffectiveness + Allocation
      │                                               ↓
      │                              actuator_motors / actuator_servos
      │                                               ↓
      └─ manual_control_switches → Command ─────→ MixingOutput
                                   ↓             ↑ CanOutput 持有它
                              actuator_armed ────┘
                                                  ↓ updateOutputs()
                                              CanOutput
                                                  ↓
                                      DamiaoProtocol → /dev/can0 → 电机
                                                  ↑ CAN 反馈
                                            actuator_status

ParamManager → parameter_update → 参数使用模块刷新参数
RobotRuntime → 启动就绪检查、低频提示、参数自动保存、队列状态
```

`Failsafe` 只发布 RC 故障状态，不直接写电机。`RobotControl` 决定故障时的目标；`Command` 决定是否允许输出；`MixingOutput` 在实际输出前再次检查状态和数据新鲜度。安全判断不是只做一次。

uORB 在本工程中是**进程内**消息层，不是 ROS 2 话题，也不是网络 DDS。不要在 Orin 上用 `ros2 topic` 指令操作这些消息。

### 3.2 构型分配

当前 `SYS_AUTOSTART=1` 选择 `1_dual_magneticwheel`，`CA_AIRFRAME=1` 选择双磁轮模型。两者是不同编号体系，只是当前值都为 1。

```text
输入 u = [speed, steering]

M0 = CA_THR_F × speed       前动力
M1 = CA_THR_R × speed       后动力
S0 = CA_YAW_F × steering    前转向
S1 = CA_YAW_R × steering    后转向
```

默认前后动力系数均为 1，前转向为 1、后转向为 0。后转向的零归一化目标对应配置位置区间的中点，不等于任何装配状态下的机械零位。该模型是轮式机器人直接映射，不是飞行器的力/力矩伪逆分配器。

### 3.3 调度模型

| 执行单元 | 模块 | 优先级 / 栈 | 运行方式 |
| --- | --- | --- | --- |
| 独立任务 | `RobotRuntime` | 100 / 8192 B | 就绪检查与低频参数保存 |
| 独立任务 | `SbusInput` | 120 / 4096 B | 串口接收 |
| 独立任务 | `CanOutput` | 115 / 6144 B | CAN 收发与输出 |
| `wq_safety` | `RcUpdate`、`Command` | 110 / 8192 B，共用队列栈 | 消息触发 + 周期兜底 |
| `wq_control` | `Failsafe`、`RobotControl`、`ControlAllocator` | 105 / 8192 B，共用队列栈 | 消息触发 + 周期兜底 |

`RcUpdate` 最小间隔与兜底间隔各为 10 ms，`Command` 和 `wq_control` 的模块各为 20 ms。消息回调请求调度，实际 `Run()` 由队列串行执行。没有新消息时，周期任务仍可检测超时。这些间隔受系统节拍和调度延迟影响。

每条队列通过信号量等候新任务或最近截止时间。待处理的唤醒通知最多保留一个，避免空闲时因通知积累持续抢占 NSH。当前 NuttX 系统节拍为 10 ms，实际检测周期和停车延迟以台架测量为准。

模块入口统一为：NSH 命令 → `ModuleBase::main()` → 模块 `task_spawn()`。队列模块构造时选择队列，`init()` 注册回调与调度；独立任务采用 `instantiate()` 与 `run()`。`param` 是一次性命令，不是常驻任务。

## 4. 当前代码框架树

```text
_code/
├─ platform/dji_cboard/                  板级支持包的正式源码
│  ├─ configs/robot/defconfig            NuttX 裁剪与串口/CAN配置
│  ├─ include/board.h                   板级公开定义
│  ├─ scripts/Make.defs                 编译工具链接入
│  ├─ scripts/ld.script                 固件与参数 Flash 分区
│  └─ src/                             初始化、CAN、LED、按键、ROMFS
├─ config/
│  ├─ versions.env                     NuttX/Apps 版本、摘要与提交固定
│  └─ nuttx/{Kconfig,Make.defs,Makefile} 应用构建与 NSH 命令注册
├─ startup/etc/
│  ├─ init.d/{rc.sysinit,rcS,rc.autostart}
│  └─ robots/1_dual_magneticwheel       构型默认参数
├─ msg/                                11 个 .msg 消息定义
│  ├─ InputRc / RcChannels / ManualControl / ManualControlSwitches
│  ├─ FailsafeStatus / RobotControlSetpoint / ParameterUpdate
│  └─ ActuatorArmed / ActuatorMotors / ActuatorServos / ActuatorStatus
├─ robot/
│  ├─ common/ModuleBase.hpp             生命周期公共入口
│  ├─ os/                              Task、Thread、Mutex、Semaphore、Clock
│  ├─ orb/                             Topics、uORB、SubscriptionCallback
│  ├─ work_queue/                      队列、工作项、定时调度与配置
│  ├─ drivers/rc/sbus.{h,cpp}           SBUS 编解码组件
│  ├─ params/                          参数声明、对象树、命令与 Flash 存储
│  ├─ configuration/RobotConfiguration.hpp
│  ├─ control/                         ArmSafety、构型模型与 Allocation
│  ├─ modules/                         每个常驻模块独立目录
│  │  ├─ robot_runtime/RobotRuntime.*
│  │  ├─ sbus_input/SbusInput.*
│  │  ├─ rc_update/{RcUpdate,RcConfig}.*
│  │  ├─ command/Command.*
│  │  ├─ failsafe/Failsafe.*
│  │  ├─ robot_control/RobotControl.*
│  │  └─ control_allocator/ControlAllocator.*
│  ├─ output/
│  │  ├─ OutputInterface.hpp / OutputFunction.hpp
│  │  ├─ MixingOutput.{hpp,cpp}
│  │  └─ can_output/CanOutput.{hpp,cpp}
│  └─ actuators/                       预留目录说明，非已实现硬件驱动
├─ protocol/can/                       CanProtocol、DamiaoProtocol
├─ tools/                              环境、下载、生成、构建和烧录脚本
├─ tests/                              主机测试与 step1～step7 检查入口
├─ docs/                               本文与专题说明
├─ upstream/{nuttx,apps}/               固定版本的上游工作源码
├─ build/                              消息头文件、ROMFS 等中间产物
└─ artifacts/                          固件、map、尺寸和构建记录
```

图中省略 `.msg` 后缀的名称仍对应 `msg/*.msg`。消息类型由 `tools/generate_messages.py` 生成到 `build/generated/msg/`；不要直接修改生成头文件。业务源码修改 `robot/`、`protocol/`，BSP 修改 `platform/`，不要只改 `upstream/apps/cboard/` 的构建副本。

## 5. 硬件接口与存储

| 用途 | 接口 / 引脚 | 当前软件配置 |
| --- | --- | --- |
| NSH 调试控制台 | USART6 TX=PG14、RX=PG9 | 115200、8N1；USB-TTL 接入 PC |
| MK32 SBUS | USART3 RX=PC11，经 CBoard DBUS 反相电路 | `/dev/ttyS2`，100000、8E2 |
| 电机 CAN | CAN1 TX=PD1、RX=PD0 | `/dev/can0`，1 Mbit/s；板端 CAN 接头 1=CANL、2=CANH |
| Orin 测厚就绪通信 | USART1 TX=PA9、RX=PB7，`/dev/ttyS1` | 115200、8N1，`cboard_link` |
| LED / 按键 | PH10、PH11、PH12 / PA0 | BSP；按键低有效 |
| PWM / CAN2 / IMU / 电池采样 | 见硬件资源表 | 不属于本版已完成控制后端 |

USB-TTL 使用 3.3 V 逻辑电平，TX/RX 交叉并共地。SBUS 和 NSH 是不同串口，不要将 115200 的控制台配置用于 SBUS。

链接脚本为固件保留起始 `0x08000000` 的 768 KiB Flash，参数使用尾部 sector 10/11 的 A/B 存储区域 `0x080C0000～0x080FFFFF`。固件 SRAM 链接区为 112 KiB，不是 MCU 全部 RAM。参数后端通过 NuttX `progmem` 访问，包含提交标志、序号与 CRC；整片擦除会清除参数。

## 6. 编译：在 PC 上执行

### 6.1 Windows PowerShell

进入下位机工程根目录：

```powershell
Set-Location 'D:\1研究生\国创磁轮检测机器人\code\Cboard\_code'
```

第一次配置 MSYS2 环境时执行；已经配置好的环境不必每次更新安装包：

```powershell
.\tools\setup_build_env.ps1
.\tools\check_env.ps1
.\tools\bootstrap.ps1
```

`setup_build_env.ps1` 使用 `C:\msys64` 安装依赖，会访问网络并更新 MSYS2。`bootstrap` 下载固定版本 NuttX/Apps 并校验 SHA-512。工具已安装时，直接检查环境并编译即可。

```powershell
# 普通源码或启动脚本修改后的编译
.\tools\build.ps1 -Jobs 4

# defconfig、编译配置切换或需要干净重建时使用
.\tools\build.ps1 -Clean -Jobs 4
```

参数：`-Clean` 请求清理 NuttX 配置后重建；`-Jobs` 控制并行度。脚本优先检测可用 WSL 工具链，不能使用时转入 MSYS2 和 STM32CubeIDE Arm GCC；MSYS2 分支最大限制为 4 个并行任务。

构建脚本自动完成：检查环境 → 下载/校验上游 → 应用固定的 NSH EOF 修正 → 生成消息 → 拷贝应用源码 → 生成 `/etc` ROMFS → 选择正式板级配置 → 编译 → 收集产物。不要手工修改 `upstream/nuttx/.config` 作为长期配置，应修改正式 `defconfig`。

### 6.2 Linux / WSL Bash

在 Linux 中进入对应工程目录，WSL 对本机 D 盘的常见映射如下：

```bash
cd '/mnt/d/1研究生/国创磁轮检测机器人/code/Cboard/_code'
bash tools/check_env.sh
bash tools/bootstrap.sh
bash tools/build.sh --jobs 4
# 干净重建：bash tools/build.sh --clean --jobs 4
```

原生 Linux 请替换首行路径。依赖包括 Bash、Python、Make、Arm GCC/G++、curl、tar、sha512sum、genromfs 及构建所需的 Kconfig 工具。缺依赖时先按环境检查结果安装，不要直接套用 Windows 烧录脚本。

### 6.3 编译产物与一致性

| 文件 | 用途 |
| --- | --- |
| `artifacts/nuttx` | ELF，包含调试信息 |
| `artifacts/nuttx.hex` | Intel HEX，现有烧录脚本默认输入 |
| `artifacts/nuttx.map` | 链接地址与符号分布 |
| `artifacts/size.txt` | text/data/bss 尺寸 |
| `artifacts/build-info.txt` | 上游版本、摘要、编译器和时间 |
| `artifacts/nuttx.bin` | 仅上游构建实际生成时才会复制；当前留存产物没有此文件 |

脚本以 `[PASS] Firmware artifacts written to ...` 结束且退出码为 0 后，再检查产物时间与构建记录。构建脚本不会自动删除所有旧产物，单看某个文件存在不够。

```powershell
Get-Item .\artifacts\nuttx, .\artifacts\nuttx.hex
Get-Content .\artifacts\build-info.txt
Get-Content .\artifacts\size.txt
Get-FileHash .\artifacts\nuttx.hex -Algorithm SHA256
git rev-parse HEAD
git status --short
```

## 7. 烧录：无线 CMSIS-DAP + OpenOCD

当前工程封装的是**正点原子高速无线调试器的 CMSIS-DAP 接口，经 SWD 烧录**，不是通过 SBUS、CAN、USB-TTL 或机载数传上传固件。板卡正常供电，按调试器定义连接 SWDIO、SWCLK、GND 及电平参考信号，接线以实际板卡和调试器说明为准。

Windows PowerShell，在工程根目录执行：

```powershell
# 默认使用 HID，经 400 kHz SWD 烧录 artifacts/nuttx.hex
.\tools\flash_wireless.ps1

# 显式指定固件与速度
.\tools\flash_wireless.ps1 -Firmware .\artifacts\nuttx.hex -AdapterKhz 400
```

| 参数 | 默认值 | 含义 |
| --- | --- | --- |
| `-Firmware` | `artifacts\nuttx.hex` | 相对工程根目录或绝对固件路径 |
| `-AdapterKhz` | `400` | SWD 时钟，允许 10～1000 kHz |

脚本在 **`C:\ST`** 下查找 STM32CubeIDE 的 `openocd.exe` 与 `st_scripts/interface/cmsis-dap.cfg`，配合 `target/stm32f4x.cfg`。其他安装目录即使能编译，也不保证烧录脚本能找到 OpenOCD。

脚本通过 HID 连接调试器，执行 `reset halt` 后按镜像擦写，再校验并重启 MCU。出现 `[PASS] Wireless flash, verification and reset completed` 表示烧录、校验和复位均已完成。源码修改后先单独编译生成新的 HEX。

烧录期间固件会暂停和复位，先让执行器处于失能状态。本工程固件区域与参数区域分离；不要额外执行整片擦除来代替脚本中的按镜像烧写，否则保存参数也会丢失。

当前没有独立封装的 ST-Link 或 Linux 烧录入口；若更换调试器，需要适配 OpenOCD 接口配置，不要把 `flash_wireless.ps1` 当作通用刷写工具。

## 8. 串口、启动与 NSH 使用

### 8.1 打开控制台

使用 USB-TTL 连接 USART6，终端设置 **115200、8N1、无流控**。Windows 查询串口：

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

可使用工程的只读诊断脚本，`COM5` 替换为实际端口；运行时先关闭占用同一端口的串口终端：

```powershell
.\tools\verify_step2_serial.ps1 -Port COM5 -DurationSeconds 8
```

该脚本发送 `uname -a`、`free`、`ps`、`mount`、`ls /proc`，默认写入 `artifacts/step2-serial.log`，检测 `nsh>`。脚本通过说明串口和 NSH 可响应，不代表 SBUS、CAN 或电机都已验收。

### 8.2 上电自动启动

```text
NuttX / NSH → ROMFS /etc/init.d/rcS
  1. robot start                        参数初始化与运行时
  2. source /etc/init.d/rc.autostart      选择 SYS_AUTOSTART 对应构型
  3. rc_update start
  4. sbus_input start -d /dev/ttyS2
  5. failsafe start
  6. robot_control start
  7. control_allocator start
  8. can_output start -d /dev/can0
  9. thickness_control start           测厚推杆控制，等待 SA 解锁
 10. cboard_link start                  上位机就绪状态同步
 11. command start
```

`RobotRuntime` 检查包括 `thickness_control` 在内的必需模块，启动成功后显示 `CBoard initialization complete`。若启动异常，执行 `robot ready` 查看缺失模块。测厚控制上电进入 `waiting_for_arm`，收到有效的 SA 解锁后才开始收回复位。使用 `thickness_control status` 查看推杆状态和测厚就绪。`robot stop` 只停止运行时模块。

### 8.3 常用诊断命令

以下输入在**开发板的 `nsh>` 提示符**，不是 Windows PowerShell 或 Orin Bash：

```sh
uname -a
free
ps
robot status
robot ready
sbus_input status
rc_update status
failsafe status
robot_control status
control_allocator status
command status
can_output status
can_output protocols
can_output map
param status
```

重点查看串口帧接收、新鲜度/失联状态、归一化输入、控制目标、解锁状态、CAN 发送错误与电机反馈。`robot status` 还提供工作队列统计。

### 8.4 安全开关与手动禁止

默认 `CA_ARM_SW=1` 使用第一路逻辑开关；它由 `RC_MAP_SW1` 映射到物理 RC 通道，默认通道为 5。实际遥控器 SA 的档位方向需核对通道映射、反向与校准，不能仅凭开关名称认定 ON/OFF。

上电必须在有效遥控中看到一次 OFF，再切换 ON 才允许解锁。`command enable` 只能解除手动禁止，不会绕过遥控互锁、数据超时或故障检查。

```sh
command disable
command status
# 确认设备和遥控状态后，解除手动禁止：
command enable
```

`can_output enable/disable` 是全局手动禁止的兼容入口。设零与清错属于会改变设备状态的维护操作，仅在失能后按电机和机械标定流程使用：

```sh
# 槽索引从 0 开始；示例不是日常启动必需步骤
can_output zero servo 0
can_output clear motor 0
```

发送使能管理帧成功不等于每个电机已反馈确认使能，必须看真实反馈。

## 9. 参数修改与保存

参数定义在 `robot/params/ParamDefinitions.cpp`。初始化后按 `ParamManager → ModuleParams` 对象树使用；更新通过 `parameter_update` 通知，Flash 保存由运行时低频处理，不放到高频控制回调。

```sh
param get SYS_AUTOSTART
param get CA_AIRFRAME
param show RC_MAP*
param show CAN_M0*
param get FS_RC_TMO

# 示例：显式设置并保存，修改前确认符合当前机械构型
param set CA_YAW_R 0.0
param save
param status
```

还支持 `set-default`、`compare`、`reset <name|all>` 和 `load`，完整语法见 `param help`。`set-default` 用于构型默认值，保留用户已有覆盖；`compare` 只比较、不修改，返回码供启动脚本条件判断。`reset all` 会恢复参数，不是普通诊断指令。

| 参数组 | 含义 / 当前默认 |
| --- | --- |
| `SYS_AUTOSTART` / `CA_AIRFRAME` | 启动配置 / 构型模型；当前均为 1 |
| `RCn_MIN/MAX/TRIM/DZ/REV` | 第 n 路 RC 校准、死区与方向，n=1～16 |
| `RC_MAP_THROTTLE/ROLL/PITCH/YAW` | 默认通道 3 / 1 / 2 / 4 |
| `RC_MAP_SW1～12` | 默认通道 5～16 |
| `CA_ARM_SW` | 默认 1，安全开关逻辑编号 |
| `CA_THR_F/R`、`CA_YAW_F/R` | 动力 / 转向分配系数 |
| `FS_RC_TMO` | 默认 0.5 s，RC 失联阈值 |
| `FS_MOT_*`、`FS_STR_*` | 失联时动力与转向策略；默认动力归零、保持转向目标 |
| `OUT_FAIL_ACT` | 公共输出数据故障处理策略 |
| `CAN_M0/1_*` | 动力物理槽；默认 ID 1/2，模式 3：速度模式 |
| `CAN_S0/1_*` | 转向物理槽；默认 ID 3/4，模式 2：位置速度模式 |

默认 ID、方向、速度和位置范围只是代码默认值，上板前必须与电机配置和机械限位一致。运行中修改 `CA_AIRFRAME` 后需要停止并重新启动分配器，不支持在正在运行的对象中直接热换构型。

## 10. 检查、测试与常见故障

### 10.1 主机测试

在 Windows PowerShell 的工程根目录执行，脚本各自完成对应阶段的结构检查或主机编译测试：

```powershell
.\tests\step1_layout.ps1
.\tests\step2_bsp.ps1
.\tests\step3_runtime.ps1
.\tests\step4_rc_input.ps1
.\tests\step5_params.ps1
.\tests\step6_can_motor.ps1
.\tests\step7_work_queue.ps1
```

覆盖 SBUS、参数、分配、安全输出、失联策略、NSH EOF、工作队列及模块启停等。它们不等于真实板卡、CAN 电气层和机械动作验证。本次文档任务没有执行这些测试或触发烧录。

### 10.2 故障定位顺序

| 现象 | 优先检查 |
| --- | --- |
| 编译找不到工具 | `check_env` 输出；是否选中了 WSL 或 MSYS2；Arm GCC 路径 |
| 改源码却行为不变 | 编译退出码、HEX 时间与摘要、烧录校验；是否只改了上游构建副本 |
| OpenOCD 找不到 | 当前烧录脚本只在 `C:\ST` 搜索，检查 CubeIDE 安装位置 |
| 烧录连接失败 | 无线调试器连接、CMSIS-DAP 枚举、SWD 接线、板卡供电及其他调试进程占用 |
| 串口无输出 | USART6、115200/8N1、TX/RX 共地、COM 选择、同端口占用及复位 |
| SBUS 无数据 | DBUS 接口、`/dev/ttyS2`、100000/8E2、接收机模式和信号反相路径 |
| 输入正常但不解锁 | `command status`、上电 OFF 观察、逻辑开关映射、手动禁止与新鲜度 |
| 有目标但电机不动 | `can_output map/status`、CAN 波特率/ID/模式、接线、反馈和独立电机电源 |
| 参数重启丢失 | `param status/save`、Flash 保存错误；是否整片擦除或启动覆盖参数 |

## 11. 扩展与文档维护

- 新机器人构型：新增 `ActuatorEffectiveness` 子类和编号启动脚本，在模型工厂、`rc.autostart`、构建列表中接入，再增加分配与安全测试。
- 新输出设备：复用 `MixingOutput` 与 `OutputInterface`，实现对应 CAN/PWM/UART 后端；不要把硬件寄存器处理放进控制分配器。
- 新下位机机构：另设状态机/闭环模块，明确就绪/故障消息；现有 RC 安全链不等于机构闭环已完成。
- 接入 Orin：先确定通信协议、资源预算和故障语义，再新增桥接模块；进程内 uORB 不自动变为跨设备消息。
- 修改功能、接口或构建方式后同步本文、对应专题与版本状态。旧阶段文档保留历史语境；其中“全部为独立任务”等描述不能覆盖当前工作队列实现。

相关说明：[代码框架树](current-code-framework-tree.md)、[当前软件框架](current-software-framework.md)、[参数专题](step5-parameters.md)、[输出专题](step6-damiao-motor-framework.md)、[硬件资源表](hardware-resource-map.md)、[10.14 重构记录](10.14修改文档.md)。涉及矛盾时，以当前源码和构建配置为准，并将实机验证结果另行记录。
