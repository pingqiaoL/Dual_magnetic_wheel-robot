# modules

- `RobotRuntime`：验证 ModuleBase 和 NuttX 线程生命周期的基础任务。
- `SbusInput`：从 USART3 接收并解析 MK32 SBUS，发布 `input_rc`。
- `RcUpdate`：校准、归一化并映射遥控通道，发布上层手动控制 topic。
- `RcConfig`：作为 `RcUpdate` 的参数子节点，保存16路校准和功能映射数组。
- `Command`：执行一次性SA上电互锁并发布 `actuator_armed`。
- `Failsafe`：只检测RC丢失并发布 `failsafe_status` 故障标志。
- `RobotControl`：订阅手动输入和故障标志，按 `FS_*` 参数生成控制目标。
- `ControlAllocator`：创建构型模型并发布逻辑 Motor/Servo 输出。

RC丢失不会直接失能执行器；`RobotControl` 持续产生安全目标。未解锁、lockdown
或输出链硬故障由 `MixingOutput` 在物理驱动前处理。
