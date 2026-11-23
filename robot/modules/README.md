# 模块组织

| 目录 | 模块职责 | 执行方式 |
|---|---|---|
| robot_runtime | 参数初始化、Flash延迟保存、汇总状态 | 独立任务100 |
| sbus_input | USART3接收SBUS，发布input_rc | 独立任务120 |
| rc_update | RcUpdate校准映射；RcConfig保存通道参数 | safety队列110 |
| command | SA上电互锁，发布actuator_armed | safety队列110 |
| failsafe | 检测RC丢失，发布failsafe_status | control队列105 |
| robot_control | 按FS_*参数生成控制目标 | control队列105 |
| control_allocator | 创建构型并分配Motor/Servo输出 | control队列105 |

CanOutput位于robot/output/can_output，使用独立任务115。

所有模块通过ModuleBase处理start/stop/status，并显式实现task_spawn。
队列模块在构造函数中选择队列，在init中注册消息回调和调度，在Run中执行一次业务；
独立任务通过task_spawn创建任务，由run_trampoline调用instantiate后执行run循环。

消息触发与备用定时检查合并；RcUpdate最小间隔及备用检查为10ms，其他队列模块为20ms。
停止时先注销回调并等待工作项执行结束，再发布安全状态并释放对象。
RC丢失由RobotControl生成安全目标；未解锁、lockdown或输出链硬故障由MixingOutput处理。
详见docs/10.14修改文档.md。
