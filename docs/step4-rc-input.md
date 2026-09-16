# 第四步：MK32 SBUS 与 rc_update

本步骤完成遥控输入链路的上半部分：

```text
MK32 遥控器
  -> MK32 接收机 S.Bus Out
  -> C 板 DBUS/SBUS 接口的硬件反相器
  -> USART3 RX PC11 (/dev/ttyS2, 100000 8E2)
  -> SbusInput / SbusDecoder
  -> input_rc
  -> RcUpdate
  -> rc_channels + manual_control + manual_control_switches
```

## 各层职责

- `robot/drivers/rc/sbus.*` 只负责 25 字节 SBUS 帧同步、16 路 11 位通道解包、丢帧和 failsafe 标志解析，不依赖 NuttX 串口。
- `robot/modules/SbusInput.*` 配置 USART3，读取串口字节并发布原始 `input_rc`。
- `msg/` 用 `.msg` 文件统一定义 `InputRc`、`RcChannels`、`ManualControl` 等全部 uORB 消息，构建时自动生成 C++ 结构体。
- `robot/orb/` 只用最新值缓存和 generation 实现轻量 uORB，使驱动层和处理层互不包含、互不调用。
- `robot/modules/RcConfig.*` 保存 16 路 MIN、MAX、TRIM、DEADZONE、REV 和功能映射。
- `robot/modules/RcUpdate.*` 以 100 Hz 轮询 `input_rc`，归一化通道并发布上层手动控制数据。连续稳定收到三帧后，控制数据才标记为有效。

默认功能映射为 CH1=roll、CH2=pitch、CH3=throttle、CH4=yaw，CH5 到 CH16 映射为十二个开关。该映射只是一套便于首次验收的默认值，最终映射应在参数系统完成后由参数加载。

## 启动顺序

ROMFS `rcS` 依次启动 `robot`、`rc_update` 和 `sbus_input`。先启动订阅者不会丢失初始化数据，因为 topic 保存发布者的最新一帧。`sbus_input` 默认读取 `/dev/ttyS2`，也可以手动指定设备：

```sh
sbus_input start -d /dev/ttyS2
```

## 主机侧测试

```powershell
.\tests\step4_rc_input.ps1
.\tools\build.ps1 -Jobs 4
```

协议测试构造已知 SBUS 帧，验证通道解包、原始值转换、frame-lost、failsafe、校准、反向和开关判定。完整构建应注册 `robot`、`sbus_input`、`rc_update` 三个 NSH 命令，并生成 `artifacts/nuttx.hex`。

## 后续板载验收

在 MK32 中把接收机输出设置为 S.Bus。将接收机 S.Bus Out 接到 C 板 DBUS/SBUS 信号端，同时共地；C 板接口已有硬件反相，软件不再反相。上电后执行：

```sh
ps
sbus_input status
rc_update status
```

`sbus_input status` 应持续增加 frames，摇杆运动应改变 raw CH1 到 CH4；`rc_update status` 应显示 `manual valid: yes`，四个归一化值位于 -1 到 1。关闭遥控器或断开接收机超过 500 ms 后，两个模块都应报告 RC lost，手动控制应变为无效。

本步骤尚未把遥控输出送入 Commander、控制器或执行器，也尚未把 RC 参数持久化到 Flash。这些模块将订阅本步骤生成的 topic，无需修改 SBUS 驱动。
