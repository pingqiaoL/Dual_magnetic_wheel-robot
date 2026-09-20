# 双磁轮构型、统一命令与CAN输出

当前实现沿用提供的PX4源码中“驱动拥有MixingOutput，MixingOutput引用驱动接口”的组织方式。
各模块使用独立NuttX task，不引入工作队列。测厚数据仍由上位机读取。

## 当前消息链路

```text
MK32 -> SbusInput -> input_rc -> RcUpdate
                              ├─ manual_control -> ControlAllocator
                              │                    -> actuator_motors/servos
                              └─ manual_control_switches -> Command
                                                           -> actuator_armed
CanOutput拥有MixingOutput -> 订阅执行器目标和actuator_armed
                         -> 检查解锁、有效性、超时、有限数值及限幅
                         -> _interface.updateOutputs(...)
                         -> CanOutput编码达妙协议 -> NuttX /dev/can0
```

ControlAllocator不再判断SA位置或生成解锁状态。电机、舵机消息的旧armed字段已移除，
唯一解锁状态定义在`msg/ActuatorArmed.msg`。所有消息头文件仍由.msg自动生成。

## 文件与职责

| 文件 | 职责 |
| --- | --- |
| robot/common/ModuleBase.hpp | CRTP创建对象、启动、停止、状态和生命周期锁 |
| robot/common/CommandRouter.hpp/.cpp | 所有扩展命令的唯一分发位置 |
| robot/modules/Command.hpp/.cpp | 遥控安全互锁、手动禁止及解锁状态发布 |
| robot/configuration/RobotConfiguration.hpp | 构型名称、两类执行器数量和可逆电机统计接口 |
| robot/control/ActuatorEffectiveness.hpp | 继承构型接口，增加参数化矩阵生成接口 |
| robot/control/ActuatorEffectivenessDualMagneticWheel.hpp/.cpp | 双磁轮模型，编号1，2个电机和2个转向执行器 |
| robot/control/Allocation.hpp/.cpp | 共用矩阵乘法、输出有效性和归一化限幅 |
| robot/modules/ControlAllocator.hpp/.cpp | 按模型编号创建子类，50Hz发布两类执行器目标 |
| robot/output/OutputInterface.hpp | CAN/PWM/UART输出回调父类 |
| robot/output/MixingOutput.hpp/.cpp | 不创建线程的公共安全输出处理器 |
| robot/output/CanOutput.hpp/.cpp | 拥有MixingOutput，管理CAN设备与协议编码/反馈 |
| protocol/can/DamiaoProtocol.hpp/.cpp | 不依赖操作系统的达妙协议编解码 |
| config/nuttx/Makefile | 源文件、NSH入口与命令名称接入 |

旧ClimbotConfiguration和ClimbotAllocation已由构型模型与共用算法替代，避免两处重复描述机型。
新增代码不使用namespace块，原CanOutput匿名namespace中的函数已改为private静态成员。
现有OS与uORB库仍沿用其os::、uorb::接口。

## command与扩展命令

ModuleBase保留start、stop、status，并在持有目标生命周期锁时调用CommandRouter。
各业务模块不再实现custom_command。CommandRouter独立的小头文件用于避免模板父类与
具体业务模块互相包含；所有扩展命令的实现都集中在其.cpp中。

```sh
command status
command can_output protocols
command can_output map
command control_allocator status
command param get CA_YAW_R
command disable
command enable
```

原来的`can_output protocols/map/enable/disable/zero/clear`和`robot ready`仍然可用，
它们也经过同一个路由。enable只解除手动禁止，不能跳过遥控互锁。
can_output enable/disable现在是全局输出禁止的兼容入口。param继续保留独立入口。

## 安全开关

沿用`CA_ARM_SW`，表示rc_update发布的第几路开关，编号1到12。
例如`CA_ARM_SW=1`使用`RC_MAP_SW1`映射的物理遥控通道；物理通道的MIN/MAX/TRIM/
REV/DEADZONE仍由rc_update处理。POSITION_ON=1、MIDDLE=2、OFF=3。

上电必须在有效遥控数据中观察到一次OFF，然后ON才解锁。上电时为ON会每5秒提示一次。
完成上电检查后，OFF或中位失能、ON使能；短暂失联只失能，不重复上电检查。
修改CA_ARM_SW或重新创建Command模块会重新要求OFF。

摇杆或开关采样超过500毫秒、映射无效、输入无效都会发布失锁。
MixingOutput也独立检查command心跳、两类目标及目标的采样时间，避免任务停止后继续使用缓存。
驱动写入失败时停止两类执行器，后续周期可重试；初次运行也发送真实失能帧，
不假定MCU复位会让独立供电的电机失能。这里的protocol armed只表示管理帧写入成功，
不代表每台电机已通过反馈确认使能。

CanOutput仍支持当前2路动力和2路转向参数槽。MixingOutput可供后续PWM/UART驱动复用，
这次未增加PWM或串口舵机的硬件驱动。

## 构型模型与分配矩阵

`ActuatorEffectivenessDualMagneticWheel`继承ActuatorEffectiveness，后者继承RobotConfiguration。
模型决定电机2路、转向2路，动力轮允许正反转。输入为u=[throttle,yaw]：

```text
a = M*u
M = [ CA_THR_F   0        ]  -> M0 前动力
    [ CA_THR_R   0        ]  -> M1 后动力
    [ 0          CA_YAW_F ]  -> S0 前转向
    [ 0          CA_YAW_R ]  -> S1 后转向
```

默认CA_THR_F/R=1、CA_YAW_F=1、CA_YAW_R=0。后转向也发布并使能，归一化目标0对应
其PMIN与PMAX的中点；如果机械零位需要对应0rad，应使用对称的位置上下限并完成电机零点校准。

这个矩阵是轮式机器人的直接控制分配矩阵，不是PX4飞行器的力/力矩有效性矩阵或其伪逆。
后续需要物理力/力矩模型时可以更换Allocation算法，模型父类与工厂选择结构已经准备好。

CA_THR/YAW参数修改后生成新矩阵。运行中改变CA_AIRFRAME会使现有分配结果无效，
需要停止并重新启动分配器；不在正在运行的任务中直接替换构型对象。

## 编号启动脚本

```text
rcS
  robot start                         # 初始化Flash参数系统
  source rc.autostart
    SYS_AUTOSTART=1
      source /etc/robots/1_dual_magneticwheel
        param set CA_AIRFRAME 1
        param set-default CA_* / CAN_*
  rc_update start
  sbus_input start -d /dev/ttyS2
  control_allocator start             # 工厂按CA_AIRFRAME创建模型
  can_output start -d /dev/can0
  command start                       # 前面的模块就绪后才接收解锁操作
  robot ready
```

SYS_AUTOSTART=1是启动配置编号，CA_AIRFRAME=1是模型编号。编号可不同，目前都使用1。
不支持的启动配置或失败的模块启动会中止rcS，未启动的command不会发布解锁。
配置文件使用set-default保留用户已有修改；CA_AIRFRAME由选中的脚本明确设置。

```sh
param get SYS_AUTOSTART
param get CA_AIRFRAME
param compare SYS_AUTOSTART 1
```

param compare不改参数，相等返回0，不相等返回非零，供NSH的if条件使用。
构建脚本对固定版本Apache apps应用`tools/fix_nsh_script_eof.py`兼容修正：正常脚本EOF
返回成功，命令错误及读取错误仍失败。修正可重复执行，不依赖手工修改下载源码。
所有启动脚本行限制在72个UTF-8字节以内，避免当前NSH行缓冲区截断。

## 增加另一机型

1. 增加`startup/etc/robots/2_<name>`，设置CA_AIRFRAME及构型参数默认值。
2. 在`rc.autostart`增加SYS_AUTOSTART=2的选择分支。
3. 新增ActuatorEffectiveness子类，实现名称、数量、可逆标志和矩阵生成。
4. 在ControlAllocator::createEffectiveness增加该CA_AIRFRAME编号的case。
5. 在config/nuttx/Makefile的CXXSRCS加入新.cpp；新的目录还需加入VPATH。

超出现有CAN的2+2参数槽时，还需扩展CanOutput容量与参数注册，或接入其他输出驱动。

## 编译、烧录与验收

这次修改了Kconfig/defconfig，首次重新配置应执行：

```powershell
.\tests\step6_can_motor.ps1
.\tools\build.ps1 -Clean -Jobs 4
.\tools\flash_wireless.ps1
```

后续只修改普通源码或构型脚本时使用`build.ps1 -Jobs 4`，再执行无线烧录脚本。
烧录后在NSH中分别输入：

```sh
robot status
command status
control_allocator status
can_output status
command can_output map
```

上电SA保持OFF，等初始化完成提示后拨ON；检查四台电机反馈及后转向中位。
主机测试覆盖协议、参数持久化、矩阵更新、开关互锁、超时、异常数值和虚函数失败处理。
真实串口、CAN时序和电机动作仍需烧录后上板验收。
