# 第三步：任务运行骨架与 OS 封装

本步骤实现不接触执行器的 C++ 运行骨架。每个后续模块使用 CRTP 继承 `ModuleBase<子类>`，使用独立 NuttX task 运行；项目不引入 PX4 WorkQueue。

## 组成

- `robot/common/ModuleBase.hpp`：仅用头文件实现 CRTP 模块父类，统一 `start`、`stop`、`status`、线程入口、对象创建和释放。
- `robot/os/Thread.*`：线程名称、优先级、栈大小、创建与回收。
- `robot/os/Mutex.*`、`Semaphore.*`、`Clock.*`：NuttX/POSIX 原语的薄封装。
- `robot/modules/RobotRuntime.*`：无硬件输出的基础运行任务，100 ms 一次循环。
- `apps/cboard/robot_main.cpp`：注册为 NSH 内置命令 `robot`。
- `startup/etc/init.d/rcS`：启动时执行 `robot start`。

构建脚本将项目自己的应用和业务源码复制到可丢弃的 `upstream/apps/cboard` 构建目录，并把 `startup/etc` 生成为板级 ROMFS。不要直接修改同步后的上游目录。

## 主机侧验证

```powershell
.\tests\step3_runtime.ps1
.\tools\build.ps1 -Clean -Jobs 4
```

成功构建时会出现 `Register: robot`，并生成新的 `artifacts/nuttx.hex`。

## 上板验收

烧录新固件后，启动日志应包含：

```text
Initializing CBoard system
Starting CBoard robot runtime
Starting CBoard robot runtime
```

在 NSH 执行：

```sh
robot status
robot start
robot stop
robot status
robot start
ps
```

预期行为：首次启动由 `rcS` 自动完成；重复 `start` 返回 already running；`stop` 后 `status` 返回 not running；再次 `start` 可以恢复运行；`ps` 中能看到 `robot_core` 线程。

这一任务只更新循环计数，不访问 CAN、PWM、UART 或任何执行器。
