# modules

- `RobotRuntime`：验证 ModuleBase 和 NuttX 线程生命周期的基础任务。
- `SbusInput`：从 USART3 接收并解析 MK32 SBUS，发布 `input_rc`。
- `RcUpdate`：校准、归一化并映射遥控通道，发布上层手动控制 topic。
- `RcConfig`：保存本阶段的遥控校准和映射默认值；后续由参数系统加载和更新。

模式管理、控制器、控制分配、执行器输出和 Flash 参数持久化将在后续步骤接入。
