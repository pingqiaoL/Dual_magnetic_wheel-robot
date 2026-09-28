# 参数速查表

本文件对应当前 `robot/params/ParamDefinitions.cpp`。固件共注册 **154 个参数**。
表中的数值是固件默认值；Flash 中已经保存的用户值会覆盖默认值。

## RC 通道校准（80个）

| 参数 | 类型 | 默认值 | 简短说明 |
| --- | --- | ---: | --- |
| `RC1_MIN` | float | 1000 | 通道1校准最小值 |
| `RC1_MAX` | float | 2000 | 通道1校准最大值 |
| `RC1_TRIM` | float | 1500 | 通道1中位值 |
| `RC1_DZ` | float | 20 | 通道1中位死区 |
| `RC1_REV` | float | 1 | 通道1方向；负数表示反向 |
| `RC2_MIN` | float | 1000 | 通道2校准最小值 |
| `RC2_MAX` | float | 2000 | 通道2校准最大值 |
| `RC2_TRIM` | float | 1500 | 通道2中位值 |
| `RC2_DZ` | float | 20 | 通道2中位死区 |
| `RC2_REV` | float | 1 | 通道2方向；负数表示反向 |
| `RC3_MIN` | float | 1000 | 通道3校准最小值 |
| `RC3_MAX` | float | 2000 | 通道3校准最大值 |
| `RC3_TRIM` | float | 1500 | 通道3中位值 |
| `RC3_DZ` | float | 20 | 通道3中位死区 |
| `RC3_REV` | float | 1 | 通道3方向；负数表示反向 |
| `RC4_MIN` | float | 1000 | 通道4校准最小值 |
| `RC4_MAX` | float | 2000 | 通道4校准最大值 |
| `RC4_TRIM` | float | 1500 | 通道4中位值 |
| `RC4_DZ` | float | 20 | 通道4中位死区 |
| `RC4_REV` | float | 1 | 通道4方向；负数表示反向 |
| `RC5_MIN` | float | 1000 | 通道5校准最小值 |
| `RC5_MAX` | float | 2000 | 通道5校准最大值 |
| `RC5_TRIM` | float | 1500 | 通道5中位值 |
| `RC5_DZ` | float | 20 | 通道5中位死区 |
| `RC5_REV` | float | 1 | 通道5方向；负数表示反向 |
| `RC6_MIN` | float | 1000 | 通道6校准最小值 |
| `RC6_MAX` | float | 2000 | 通道6校准最大值 |
| `RC6_TRIM` | float | 1500 | 通道6中位值 |
| `RC6_DZ` | float | 20 | 通道6中位死区 |
| `RC6_REV` | float | 1 | 通道6方向；负数表示反向 |
| `RC7_MIN` | float | 1000 | 通道7校准最小值 |
| `RC7_MAX` | float | 2000 | 通道7校准最大值 |
| `RC7_TRIM` | float | 1500 | 通道7中位值 |
| `RC7_DZ` | float | 20 | 通道7中位死区 |
| `RC7_REV` | float | 1 | 通道7方向；负数表示反向 |
| `RC8_MIN` | float | 1000 | 通道8校准最小值 |
| `RC8_MAX` | float | 2000 | 通道8校准最大值 |
| `RC8_TRIM` | float | 1500 | 通道8中位值 |
| `RC8_DZ` | float | 20 | 通道8中位死区 |
| `RC8_REV` | float | 1 | 通道8方向；负数表示反向 |
| `RC9_MIN` | float | 1000 | 通道9校准最小值 |
| `RC9_MAX` | float | 2000 | 通道9校准最大值 |
| `RC9_TRIM` | float | 1500 | 通道9中位值 |
| `RC9_DZ` | float | 20 | 通道9中位死区 |
| `RC9_REV` | float | 1 | 通道9方向；负数表示反向 |
| `RC10_MIN` | float | 1000 | 通道10校准最小值 |
| `RC10_MAX` | float | 2000 | 通道10校准最大值 |
| `RC10_TRIM` | float | 1500 | 通道10中位值 |
| `RC10_DZ` | float | 20 | 通道10中位死区 |
| `RC10_REV` | float | 1 | 通道10方向；负数表示反向 |
| `RC11_MIN` | float | 1000 | 通道11校准最小值 |
| `RC11_MAX` | float | 2000 | 通道11校准最大值 |
| `RC11_TRIM` | float | 1500 | 通道11中位值 |
| `RC11_DZ` | float | 20 | 通道11中位死区 |
| `RC11_REV` | float | 1 | 通道11方向；负数表示反向 |
| `RC12_MIN` | float | 1000 | 通道12校准最小值 |
| `RC12_MAX` | float | 2000 | 通道12校准最大值 |
| `RC12_TRIM` | float | 1500 | 通道12中位值 |
| `RC12_DZ` | float | 20 | 通道12中位死区 |
| `RC12_REV` | float | 1 | 通道12方向；负数表示反向 |
| `RC13_MIN` | float | 1000 | 通道13校准最小值 |
| `RC13_MAX` | float | 2000 | 通道13校准最大值 |
| `RC13_TRIM` | float | 1500 | 通道13中位值 |
| `RC13_DZ` | float | 20 | 通道13中位死区 |
| `RC13_REV` | float | 1 | 通道13方向；负数表示反向 |
| `RC14_MIN` | float | 1000 | 通道14校准最小值 |
| `RC14_MAX` | float | 2000 | 通道14校准最大值 |
| `RC14_TRIM` | float | 1500 | 通道14中位值 |
| `RC14_DZ` | float | 20 | 通道14中位死区 |
| `RC14_REV` | float | 1 | 通道14方向；负数表示反向 |
| `RC15_MIN` | float | 1000 | 通道15校准最小值 |
| `RC15_MAX` | float | 2000 | 通道15校准最大值 |
| `RC15_TRIM` | float | 1500 | 通道15中位值 |
| `RC15_DZ` | float | 20 | 通道15中位死区 |
| `RC15_REV` | float | 1 | 通道15方向；负数表示反向 |
| `RC16_MIN` | float | 1000 | 通道16校准最小值 |
| `RC16_MAX` | float | 2000 | 通道16校准最大值 |
| `RC16_TRIM` | float | 1500 | 通道16中位值 |
| `RC16_DZ` | float | 20 | 通道16中位死区 |
| `RC16_REV` | float | 1 | 通道16方向；负数表示反向 |

## RC 功能映射（17个）

| 参数 | 类型 | 默认值 | 简短说明 |
| --- | --- | ---: | --- |
| `RC_MAP_THROTTLE` | int32 | 3 | 油门使用的物理通道 |
| `RC_MAP_ROLL` | int32 | 1 | 横滚使用的物理通道 |
| `RC_MAP_PITCH` | int32 | 2 | 俯仰使用的物理通道 |
| `RC_MAP_YAW` | int32 | 4 | 偏航/转向使用的物理通道 |
| `RC_MAP_SW1` | int32 | 5 | 逻辑开关1使用的物理通道 |
| `RC_MAP_SW2` | int32 | 6 | 逻辑开关2使用的物理通道 |
| `RC_MAP_SW3` | int32 | 7 | 逻辑开关3使用的物理通道 |
| `RC_MAP_SW4` | int32 | 8 | 逻辑开关4使用的物理通道 |
| `RC_MAP_SW5` | int32 | 9 | 逻辑开关5使用的物理通道 |
| `RC_MAP_SW6` | int32 | 10 | 逻辑开关6使用的物理通道 |
| `RC_MAP_SW7` | int32 | 11 | 逻辑开关7使用的物理通道 |
| `RC_MAP_SW8` | int32 | 12 | 逻辑开关8使用的物理通道 |
| `RC_MAP_SW9` | int32 | 13 | 逻辑开关9使用的物理通道 |
| `RC_MAP_SW10` | int32 | 14 | 逻辑开关10使用的物理通道 |
| `RC_MAP_SW11` | int32 | 15 | 逻辑开关11使用的物理通道 |
| `RC_MAP_SW12` | int32 | 16 | 逻辑开关12使用的物理通道 |
| `RC_CHAN_CNT` | int32 | 16 | 注册的RC通道数量；当前处理固定为16路 |

## 系统与控制分配（7个）

| 参数 | 类型 | 默认值 | 简短说明 |
| --- | --- | ---: | --- |
| `SYS_AUTOSTART` | int32 | 1 | 启动构型编号；1为双磁轮机器人 |
| `CA_AIRFRAME` | int32 | 1 | 控制分配模型编号；1为双磁轮模型 |
| `CA_THR_F` | float | 1.0 | 油门到前动力轮的分配系数 |
| `CA_THR_R` | float | 1.0 | 油门到后动力轮的分配系数 |
| `CA_YAW_F` | float | 1.0 | 偏航到前转向轮的分配系数 |
| `CA_YAW_R` | float | 0.0 | 偏航到后转向轮的分配系数 |
| `CA_ARM_SW` | int32 | 1 | 用于解锁的逻辑开关编号，范围1～12 |

## Failsafe 与输出安全（6个）

| 参数 | 类型 | 默认值 | 简短说明 |
| --- | --- | ---: | --- |
| `FS_RC_TMO` | float | 0.5 | RC超时时间，单位秒，有效范围0.05～10 |
| `FS_MOT_ACT` | int32 | 1 | RC丢失后的动力目标策略 |
| `FS_MOT_VAL` | float | 0.0 | 动力策略选择固定值时使用的目标 |
| `FS_STR_ACT` | int32 | 0 | RC丢失后的转向目标策略 |
| `FS_STR_VAL` | float | 0.0 | 转向策略选择固定值时使用的目标 |
| `OUT_FAIL_ACT` | int32 | 2 | 执行器topic异常时的底层输出策略 |

## CAN 动力电机（20个）

| 参数 | 类型 | 默认值 | 简短说明 |
| --- | --- | ---: | --- |
| `CAN_M0_FUNC` | int32 | 101 | 前动力轮绑定的逻辑输出功能 |
| `CAN_M0_PROTO` | int32 | 1 | 前动力轮使用的CAN协议 |
| `CAN_M0_TYPE` | int32 | 2 | 前动力轮的达妙电机型号标识 |
| `CAN_M0_ID` | int32 | 1 | 前动力轮的控制ID |
| `CAN_M0_FBID` | int32 | 0 | 前动力轮的反馈帧ID；0表示不过滤帧ID |
| `CAN_M0_MODE` | int32 | 3 | 前动力轮的达妙控制模式 |
| `CAN_M0_REV` | float | 1.0 | 前动力轮输出方向，只允许1或-1 |
| `CAN_M0_PMAX` | float | 12.5 | 前动力轮反馈位置解码量程，单位rad |
| `CAN_M0_VMAX` | float | 30.0 | 前动力轮最大速度及反馈速度量程，单位rad/s |
| `CAN_M0_TMAX` | float | 10.0 | 前动力轮反馈力矩量程，单位N·m |
| `CAN_M1_FUNC` | int32 | 102 | 后动力轮绑定的逻辑输出功能 |
| `CAN_M1_PROTO` | int32 | 1 | 后动力轮使用的CAN协议 |
| `CAN_M1_TYPE` | int32 | 2 | 后动力轮的达妙电机型号标识 |
| `CAN_M1_ID` | int32 | 2 | 后动力轮的控制ID |
| `CAN_M1_FBID` | int32 | 0 | 后动力轮的反馈帧ID；0表示不过滤帧ID |
| `CAN_M1_MODE` | int32 | 3 | 后动力轮的达妙控制模式 |
| `CAN_M1_REV` | float | 1.0 | 后动力轮输出方向，只允许1或-1 |
| `CAN_M1_PMAX` | float | 12.5 | 后动力轮反馈位置解码量程，单位rad |
| `CAN_M1_VMAX` | float | 30.0 | 后动力轮最大速度及反馈速度量程，单位rad/s |
| `CAN_M1_TMAX` | float | 10.0 | 后动力轮反馈力矩量程，单位N·m |

## CAN 转向电机（24个）

| 参数 | 类型 | 默认值 | 简短说明 |
| --- | --- | ---: | --- |
| `CAN_S0_FUNC` | int32 | 201 | 前转向轮绑定的逻辑输出功能 |
| `CAN_S0_PROTO` | int32 | 1 | 前转向轮使用的CAN协议 |
| `CAN_S0_TYPE` | int32 | 1 | 前转向轮的达妙电机型号标识 |
| `CAN_S0_ID` | int32 | 3 | 前转向轮的控制ID |
| `CAN_S0_FBID` | int32 | 0 | 前转向轮的反馈帧ID；0表示不过滤帧ID |
| `CAN_S0_MODE` | int32 | 2 | 前转向轮的达妙控制模式 |
| `CAN_S0_REV` | float | 1.0 | 前转向轮输出方向，只允许1或-1 |
| `CAN_S0_PMIN` | float | -1.0 | 前转向轮归一化-1对应的最小角度，单位rad |
| `CAN_S0_PMAX` | float | 1.0 | 前转向轮归一化+1对应的最大角度，单位rad |
| `CAN_S0_PFBMAX` | float | 12.5 | 前转向轮反馈位置解码量程，单位rad |
| `CAN_S0_VMAX` | float | 5.0 | 前转向轮位置速度模式目标速度，单位rad/s |
| `CAN_S0_TMAX` | float | 10.0 | 前转向轮反馈力矩量程，单位N·m |
| `CAN_S1_FUNC` | int32 | 202 | 后转向轮绑定的逻辑输出功能 |
| `CAN_S1_PROTO` | int32 | 1 | 后转向轮使用的CAN协议 |
| `CAN_S1_TYPE` | int32 | 1 | 后转向轮的达妙电机型号标识 |
| `CAN_S1_ID` | int32 | 4 | 后转向轮的控制ID |
| `CAN_S1_FBID` | int32 | 0 | 后转向轮的反馈帧ID；0表示不过滤帧ID |
| `CAN_S1_MODE` | int32 | 2 | 后转向轮的达妙控制模式 |
| `CAN_S1_REV` | float | 1.0 | 后转向轮输出方向，只允许1或-1 |
| `CAN_S1_PMIN` | float | -1.0 | 后转向轮归一化-1对应的最小角度，单位rad |
| `CAN_S1_PMAX` | float | 1.0 | 后转向轮归一化+1对应的最大角度，单位rad |
| `CAN_S1_PFBMAX` | float | 12.5 | 后转向轮反馈位置解码量程，单位rad |
| `CAN_S1_VMAX` | float | 5.0 | 后转向轮位置速度模式目标速度，单位rad/s |
| `CAN_S1_TMAX` | float | 10.0 | 后转向轮反馈力矩量程，单位N·m |

## 枚举值速查

- `FS_MOT_ACT`、`FS_STR_ACT`：`0`保持最后值，`1`归零，`2`使用对应的 `FS_*_VAL`。
- `OUT_FAIL_ACT`：`0`立即失能，`1`持续发送归零目标，`2`归零500 ms后失能。
- `CAN_*_FUNC`：`0`禁用，`101` Motor1，`102` Motor2，`201` Servo1，`202` Servo2。
- `CAN_*_PROTO`：`0`禁用，`1`达妙协议。
- `CAN_*_TYPE`：`0`未知，`1` DM4310，`2` DMS3519。当前仅作型号标识。
- `CAN_*_MODE`：`1` MIT，`2`位置速度，`3`速度，`4`位置速度力矩。当前使用动力模式3、转向模式2。
- `CAN_*_REV`：`1`正向，`-1`反向。

## param 操作指令

```sh
# 查看参数系统状态
param status

# 查看全部参数
param show

# 按前缀查看
param show RC1_*
param show RC_MAP_*
param show CA_*
param show FS_*
param show CAN_M0_*
param show CAN_S0_*

# 查看一个参数
param get CAN_S0_PMAX

# 修改当前值
param set CAN_S0_PMAX 0.523599

# 设置构型默认值
param set-default CA_YAW_R 0

# 比较参数；相等返回0，供NSH脚本的if使用
param compare SYS_AUTOSTART 1

# 恢复一个参数到当前默认值
param reset CAN_S0_PMAX

# 恢复全部参数到当前默认值
param reset all

# 立即保存到Flash
param save

# 从Flash重新加载
param load
```

构型脚本使用 `param set-default` 设置机型默认值，用户通过 `param set` 保存的值优先。
运行中修改普通参数会发布 `parameter_update`；修改 `CA_AIRFRAME` 后应重启 `control_allocator`。
