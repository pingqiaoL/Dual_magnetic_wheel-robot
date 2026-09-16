# 执行器输出接口

`OutputInterface.hpp` 是 PWM、CAN 和 UART 输出共同继承的最小接口。
`CanOutput` 是当前第一种实现：它订阅 `actuator_motors` 与
`actuator_servos`，根据 `CAN_M*_*`、`CAN_S*_*` 参数选择协议，并通过
NuttX `/dev/can0` 收发标准 CAN 帧。

协议编解码位于顶层 `protocol/can/`，不依赖 NuttX 和 uORB，因此可以在
Windows 主机上单独测试。启动任务不会自动使能电机；必须执行
上电后必须先将 `CA_ARM_SW` 指定的遥控开关置于 OFF，再拨到 ON。
`can_output` 默认允许安全互锁控制输出；`can_output disable` 只作为调试时的手动禁止。
