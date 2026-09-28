# 执行器输出接口

`OutputInterface.hpp` 是 PWM、CAN 和 UART 输出共同继承的最小接口。
每个物理驱动拥有一个 `MixingOutput`，由它订阅 `actuator_motors` 和
`actuator_servos`，再使用 `OutputFunction` 把逻辑执行器映射成驱动自己的
物理通道数组。`101/102` 表示 Motor1/2，`201/202` 表示 Servo1/2。

当前只有 `CanOutput` 物理驱动。四个槽位使用 `CAN_M0_FUNC`、
`CAN_M1_FUNC`、`CAN_S0_FUNC`、`CAN_S1_FUNC` 选择逻辑执行器，协议、ID、
模式和量程仍由同组 `CAN_*` 参数配置。保留 M/S 参数名是为了兼容已经
保存到 Flash 的电机配置。未来 PWM 驱动可复用同一个 `MixingOutput`，
无需修改控制分配或 Failsafe。

协议编解码位于顶层 `protocol/can/`，不依赖 NuttX 和 uORB，因此可以在
Windows 主机上单独测试。上电后必须先将 `CA_ARM_SW` 指定的遥控开关
置于 OFF，再拨到 ON。
`can_output` 默认允许安全互锁控制输出；`can_output disable` 只作为调试时的手动禁止。

RC丢失由`Failsafe`发布标志，再由`RobotControl`持续发布安全目标。输出Topic异常属于更深层
故障，`OUT_FAIL_ACT=2` 时先发送归一化零值 500 ms，再物理失能。
