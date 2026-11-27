# 电推杆驱动与 SF 控制

日期为 2026-11-24。普通直流电推杆通过 DRV8871 驱动。回收采用定时停车，下压由一个常开工作开关触发停车，途中可随时停止。

## 文件与分工

| 文件 | 作用 |
| --- | --- |
| `robot/drivers/probe_lift/ProbeLift.hpp/.cpp` | 三个动作、方向参数、到位确认、超时及故障处理 |
| `platform/dji_cboard/include/probe_lift.h` | C 板 GPIO 操作接口，也供主机测试替换实现 |
| `platform/dji_cboard/src/stm32_probe_lift.c` | STM32 GPIO 初始化、开关采样、DRV8871 输出 |
| `platform/dji_cboard/src/dji_cboard.h` | 定义三路 GPIO |
| `platform/dji_cboard/src/Make.defs` | 编译板级驱动 |
| `robot/modules/thickness_control/ThicknessControl.hpp/.cpp` | 根据解锁状态和 SF 控制推杆 |
| `robot/modules/thickness_control/ProbeLiftBench.cpp` | NSH 开关检测和限时点动测试 |
| `robot/modules/rc_update/RcUpdate.cpp` | 解算三档拨杆，缺失的通道输出 `POSITION_NONE` |
| `msg/ThicknessControlStatus.msg`、`robot/orb/Topics.hpp/.cpp` | 发布 C 板内部状态与测厚就绪 |
| `robot/params/ParamDefinitions.cpp` | 注册方向、下压超时、复位时间、C5 确认时间和 SF 映射 |
| `config/nuttx/Makefile`、`Kconfig` | 编译驱动并注册 NSH 命令 |
| `tests/probe_lift_test.cpp`、`tests/thickness_control_test.cpp` | 模拟 GPIO 和单调时钟，验证驱动和 SF 控制 |

板级代码负责 GPIO，`ProbeLift` 负责执行动作，`ThicknessControl` 根据已有 SA 解锁、失联状态和 SF 档位决定何时发出动作。上位机通信模块 `cboard_link` 订阅 `thickness_control_status`，经 USART1 同步就绪标志。首次按下 C5 由推杆驱动计时，连续按下达到参数值后才发布就绪。

## 接线与假设

2026-11-25 起工作开关使用 J16 的 C5（PC6）。常开开关接 C5 和 A 排 GND。

| C 板 J16 | 用途 | 配置 |
| --- | --- | --- |
| C2，PE11 | DRV8871 IN1 | 推挽输出 |
| C3，PE13 | DRV8871 IN2 | 推挽输出 |
| C5，PC6 | 工作开关一根线 | 上拉输入，按下为低 |
| A 排，GND | 工作开关另一根线及 DRV8871 信号地 | 共地 |

驱动将 J16 的 C2、C3、C5 配置为普通 GPIO。推杆动力电源经驱动桥 POWER 端供电，MOTOR 两端接电机。

工作开关确认探头已压到工作位置。`ResetDone` 仅表示设定的收缩时长已执行完，实际收回位置取决于复位时长标定和推杆自身限位。

## 参数

| 名称 | 类型 | 默认值 | 含义 |
| --- | --- | --- | --- |
| `LIFT_REV` | int32 | 1 | 1 时上升使用 IN1=1、IN2=0，-1 时交换极性。下降使用相反极性 |
| `LIFT_TIMEOUT` | float | 10.0 | 下压动作的最长时间，单位秒，范围 0.1～120 |
| `LIFT_RESET_TIME` | float | 10.0 | 收缩驱动时长，单位秒。SF 控制要求 0.1～120 |
| `LIFT_C4_STABLE` | int32 | 50 | C5 连续按下确认时间，单位毫秒，范围 1～1000。保留历史参数名以兼容已保存设置 |
| `LIFT_SF_SW` | int32 | 6 | SF 逻辑开关编号，范围 1～12。默认 `RC_MAP_SW6=10`，对应 MK32 的 CH10 |

下压超时从收到命令开始计时，包含启动制动间隔和开关确认。回收从实际输出收缩方向开始计时。复位时长为 0 时，独立驱动测试仍可跳过自动回收，SF 控制会在解锁时报告 `InvalidParameter`。

推杆方向和时间在 SA 解锁时读取。修改后先锁定，再重新解锁。SF 逻辑编号在模块启动时读取，修改后重启 `thickness_control`。已保存的参数覆盖固件默认值，可通过 `param get LIFT_RESET_TIME` 确认当前设定。

先在台架确认收缩极性和全行程用时，再用 NSH 设置并保存。将下方占位项替换为实测值。

```sh
param set LIFT_REV <1或-1>
param set LIFT_RESET_TIME <实测秒数>
param set LIFT_C4_STABLE 50
param save
```

使用定时回收前，确认推杆在收缩端会自行断电，或确认厂家允许该工况的堵转时间。单纯的机械止挡可能使电机持续堵转直至超时。

## NSH 操作

`rcS` 在 `command` 之前自动启动测厚控制。上电时保持 SA 锁定，用 `thickness_control status` 确认进入 `waiting_for_arm`。如需诊断启动异常，执行 `robot ready`。

```sh
rc_update status
param get LIFT_SF_SW
param get LIFT_RESET_TIME
thickness_control status
```

朝上应显示 `up`，中位显示 `middle`，朝下显示 `down`。如果物理档位和显示相反，核对 MK32 映射及 C 板上所用物理通道的 `RCx_REV`。停止模块时执行 `thickness_control stop`，推杆制动并清除就绪。台架点动结束后，保持 SA 锁定并用 `thickness_control start` 恢复控制，也可重启 C 板。

| 阶段 | 操作与结果 |
| --- | --- |
| `waiting_for_arm` | 观察到 SA 锁定后，下一次有效解锁直接开始初始复位，任意 SF 初始档位均可 |
| `homing` | 按设置时间收缩，忽略 SF 换档 |
| `waiting_for_first_work` | 只接受复位完成后新的上→下或中→下档位变化。若已在下档，先回上或中 |
| `manual_control` | 上档定时回收，中档制动，下档下压到 C5 |
| `fault` | 停车并显示故障，排查后锁定再解锁 |

解除解锁、手动锁定和整条遥控链路失联都会停车并清除测厚就绪。初始复位被打断时需锁定再解锁。若在等待首次下拨时失联，恢复后重新读取上档或中档作为下拨基准。正常三档期间失联或 SF 通道缺失，恢复后先回中再接受新指令。

## 台架测试

烧录本节构建的固件后，从 C 板 NSH 按下面顺序测试。前两步保持 SA 锁定，并停止正式推杆控制模块。

### 1. 只测工作开关

断开推杆动力供电，保留 C 板供电。在 10 秒采样窗口内按下和松开 C5 开关。

```sh
thickness_control stop
thickness_control test switch
```

按下应显示 `pressed (LOW)`，松开应显示 `released (HIGH)`。两种变化都捕获到时会打印 `PASS`。若始终显示松开，核对常开开关两端是否接 C5 和 A 排 GND。

### 2. 测方向和限位停车

让机构离开行程端点，接回推杆动力电源。在 NSH 用 200 毫秒短点动确认收缩和伸出。

```sh
param get LIFT_REV
thickness_control test up 200
thickness_control test down 200
```

`up` 应向上收缩，`down` 应向下伸出。若方向相反，将 `LIFT_REV` 翻转后重试，确认后再保存。以当前值是 1 为例，可执行 `param set LIFT_REV -1`，确认后执行 `param save`。每次点动会重新读取参数。点动参数范围为 50～1000 毫秒，时间包含启动前的制动间隔。

先按住 C5 再执行 `thickness_control test down 200`，推杆应保持停车并打印 `Stopped: C5 work switch confirmed.`。然后在下压过程中触发开关并保持按下，验证推杆在连续按下达到设定时间后停车。未触发开关时，由点动时间结束动作并制动。

### 3. 集成验证 SA 和 SF

确认轮系在台架上可安全解锁、推杆端部限位有效。将 SA 放在锁定档，重启 C 板，确认 `robot ready` 通过且 `thickness_control status` 显示 `phase: waiting_for_arm` 和 `measurement ready: no`。此时推杆应静止。

1. 把 SF 放在下档再解锁 SA。应立即开始初始回收，默认 10 秒到时停车。回收中拨动 SF，应继续完成回收。
2. 复位结束时把 SF 保持在下档，确认没有自行下压。拨到上档或中档，再拨到下档，应开始下压。
3. 下压触发 C5 后，推杆继续下压，直到 C5 连续按下达到 `LIFT_C4_STABLE` 才制动。`thickness_control status` 应显示 `lift: work_ready` 和 `measurement ready: yes`。台架上确认这段额外行程在弹簧和开关允许范围内。
4. 模拟工作就绪后松开 C5，保持 SF 下档。应显示 `lift: stopped`、`error: none` 和 `measurement ready: no`，推杆保持停车。SF 回中后再拨下，应重新开始下压。
5. 验证上档回收、中档停车，并确认 SA 锁定时推杆停止、测厚就绪清零。

测试结束可执行 `thickness_control stop`，推杆制动。

## 驱动接口

| 接口 | 行为 |
| --- | --- |
| `bool init(bool requireResetTime = false)` | 初始化 GPIO 并读取参数。已配置复位时长时发起回收。SF 控制传入 true |
| `bool initStopped()` | 初始化 GPIO 并读取参数，保持制动，供点动测试使用 |
| `void work()` | 请求下降到工作开关 |
| `void reset()` | 请求收缩到设定时间后停车 |
| `void stop()` | 立即制动并撤销就绪。已有错误保持锁存 |
| `void update()` | 周期处理工作开关、复位计时和下压超时 |
| `Status status() const` | 读取当前动作或就绪状态 |
| `Error error() const` | 读取错误原因 |

控制模块每 5 毫秒调度一次，同一工作队列串行调用驱动。实际停车延迟还包括调度、驱动及机械响应。工作开关稳定触发后，`thickness_control_status.measurementReady` 为真。停车、回收和故障都会使该值恢复为假。

## 状态规则

```text
init / reset → MovingToReset → 收缩时间到 → ResetDone
work → MovingToWork  → 工作开关稳定 → WorkReady
stop → Stopped
异常 → 制动 → Error
```

- 新运动先制动 20 毫秒，再施加指定极性，直接换向也经过这段间隔。该间隔是软件默认值，实际机械停稳时间待测。
- 下压过程持续驱动，首次采到 C5 按下开始计时。计时期间采到松开则清零，继续下压。连续按下达到 `LIFT_C4_STABLE` 后制动并发布就绪。
- 电机开始下压前，若 C5 已按住则保持制动并计时确认。`reset()` 允许从开关按住的位置收回。
- 重复请求同一动作会沿用原计时。动作已完成时继续保持停止。
- 工作就绪后，C5 持续松开 20 毫秒会撤销就绪并进入 `Stopped`。保持下档时推杆继续停车。回中再拨下可以重新工作。
- `Error` 状态下保持制动并保留故障。排查后通过 `init()` 重新初始化。

| Error | 含义 |
| --- | --- |
| None | 无错误 |
| Initialization | GPIO 初始化失败，或初始化前请求运动 |
| InvalidParameter | 参数读取失败、取值非法或复位时间为 0 时请求复位 |
| Timeout | 下压超时 |

工作开关断线时会一直读为松开，驱动通过下压超时结束运动。下压超时值需要结合机构允许行程与停止余量标定。

## 测试与编译

在项目根目录的 PowerShell 运行以下命令。

```powershell
.\tests\probe_lift.ps1
.\tests\step5_params.ps1
.\tests\step7_work_queue.ps1
.\tools\build.ps1 -Clean -Jobs 4
```

测试覆盖定时复位、正反映射、C4 确认时间与抖动、松开后撤销就绪及再次下拨、首次下拨、失联恢复和 NSH 生命周期。

MSYS2 下 NuttX 的板级目录使用文件副本。修改板级 GPIO 或头文件后使用 `-Clean` 刷新旧副本。

C4 稳定确认改动的构建日志位于 `build/probe_lift_c4_stable_build.log`，烧录文件为 `artifacts/nuttx.hex`。

C4 开关、下压和停车已由实机台架测试确认。加入自启动后的冷启动验证见本文件的集成验证步骤。

