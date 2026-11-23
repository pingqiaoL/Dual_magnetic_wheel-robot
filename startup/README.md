# Startup scripts

`rcS` is compiled into the `/etc` ROMFS. `robot start` initializes parameters. Each queued module creates its own
object in task_spawn(), registers callbacks in init(), and lazily starts its
configured queue when the first item is constructed. SBUS, CAN output and RobotRuntime remain independent
tasks. See `docs/10.14修改文档.md` for the scheduling and shutdown design.
