# 源码目录规划

第一步只建立边界和命名，具体 C++ 类将在后续步骤中逐层实现。

```text
_code/
├─ apps/                       # 机器人应用与 NSH 可启动命令
├─ msg/                        # 所有 uORB .msg 源文件
├─ robot/                      # 与 NuttX BSP 解耦的 C++ 业务代码
│  ├─ common/                 # 时间、数学、返回值等公共设施
│  ├─ orb/                    # 轻量发布/订阅总线
│  ├─ params/                 # 参数定义、校验、Flash 持久化
│  ├─ actuators/              # 电机、PWM 舵机、串口舵机抽象
│  ├─ output/                 # OutputInterface 与 MixingOutput
│  ├─ modules/                # 遥控、控制、执行器、安全等任务
│  └─ os/                     # 线程、同步、时间的薄封装
├─ platform/dji_cboard/       # STM32F407IGH6 板级配置与驱动适配
├─ protocol/                  # MAVLink、SBUS 和设备协议
├─ startup/                   # NSH 启动脚本及构型选择
├─ config/                    # 固定版本和硬件资源清单
├─ tools/                     # 环境检查、拉取和构建脚本
├─ tests/                     # 主机侧结构测试及后续单元测试
├─ upstream/                  # 固定提交的 NuttX 和 apps（不入库）
├─ build/                     # 构建中间文件
└─ artifacts/                 # 固件、map 和构建记录
```

## 依赖方向

```text
apps/modules ──> robot interfaces ──> platform drivers ──> NuttX
      │                 │
      └────────> msg + orb <────────┘

protocol parsers ──> msg 中的普通数据结构 ──> orb
```

- `robot/` 不直接包含 STM32 寄存器头文件。
- `msg/` 只保存简洁的 `.msg` 数据定义；C++ 头文件由构建脚本自动生成。
- `platform/dji_cboard/` 负责把 NuttX 设备和驱动封装成业务层接口。
- `OutputInterface` 由 `CanOutput`、`PwmOutput`、`UartOutput` 继承；构型只通过 `MixingOutput` 组合执行器。
- 下位机测厚仪驱动不进入此树；测厚数据由上位机读取。
- 机器人构型、执行器数量和通道映射放在构型配置中，不散落在驱动代码内。
