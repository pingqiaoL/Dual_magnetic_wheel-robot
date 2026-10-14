# Startup scripts

`rcS` is compiled into the `/etc` ROMFS. `robot start` initializes parameters
and the safety (110) and control (105) work queues before the five queued
modules are started. SBUS, CAN output and RobotRuntime remain independent
tasks. See `docs/10.13修改文档.md` for the scheduling and shutdown design.
