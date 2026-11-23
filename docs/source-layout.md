# 源码目录规划

当前目录按构型模型、安全管理、分配算法、消息和输出驱动划分职责。

```text
_code/
├─ msg/                        # 所有 uORB .msg 源文件
├─ robot/                      # 与 NuttX BSP 解耦的 C++ 业务代码
│  ├─ common/                 # CRTP ModuleBase生命周期父类
│  ├─ orb/                    # 轻量发布/订阅总线
│  ├─ params/                 # 参数定义、校验、Flash 持久化
│  ├─ configuration/         # RobotConfiguration统计父类
│  ├─ control/               # ActuatorEffectiveness模型与Allocation算法
│  ├─ output/                 # OutputInterface 与 MixingOutput
│  ├─ modules/                # 每个模块独立目录，统一NSH生命周期入口
│  ├─ work_queue/             # 队列配置、管理、WorkItem和定时执行引擎
│  └─ os/                     # 线程、同步、时间的薄封装
├─ platform/dji_cboard/       # STM32F407IGH6 板级配置与驱动适配
├─ protocol/                  # MAVLink、SBUS 和设备协议
├─ startup/                   # NSH 启动脚本及构型选择
├─ config/                    # 固定版本、硬件清单、nuttx构建接入
├─ tools/                     # 环境检查、拉取和构建脚本
├─ tests/                     # 主机侧结构测试及后续单元测试
├─ upstream/                  # 固定提交的 NuttX 和 apps（不入库）
├─ build/                     # 构建中间文件
└─ artifacts/                 # 固件、map 和构建记录
```

## 依赖方向

```text
robot/modules ──> robot interfaces ──> platform drivers ──> NuttX
      │                 │
      └────────> msg + orb <────────┘

protocol parsers ──> msg 中的普通数据结构 ──> orb
```

- `robot/` 不直接包含 STM32 寄存器头文件。
- `msg/` 只保存简洁的 `.msg` 数据定义；C++ 头文件由构建脚本自动生成。
- `platform/dji_cboard/` 负责把 NuttX 设备和驱动封装成业务层接口。
- `OutputInterface` 由 `CanOutput`、`PwmOutput`、`UartOutput` 继承；驱动拥有 `MixingOutput` 成员，成员引用驱动接口，公共处理后通过虚函数回调。
- 下位机测厚仪驱动不进入此树；测厚数据由上位机读取。
- 机器人构型、执行器数量和通道映射放在构型配置中，不散落在驱动代码内。

扩展命令由各模块自己的`custom_command()`处理；解锁管理在`robot/modules/command/Command.cpp`。
完整启动顺序、模型增加方式和测试说明见`step6-damiao-motor-framework.md`。

## 模块创建与目录

`robot_runtime/`、`sbus_input/`使用独立任务；`rc_update/`、`command/`、
`failsafe/`、`robot_control/`、`control_allocator/`使用工作队列。
CAN独立任务位于`robot/output/can_output/`，RcConfig与RcUpdate同目录。

共同入口为`module_main → ModuleBase::main → task_spawn`。
队列模块的构造函数选择队列，task_spawn创建对象后调用init，工作队列执行Run；
独立任务的task_spawn指定优先级和栈，run_trampoline调用instantiate后执行run。
详见`10.14修改文档.md`。
