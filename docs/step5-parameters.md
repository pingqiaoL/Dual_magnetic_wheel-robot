# 第五步：参数系统

参数系统由 `ParamManager`、`ParamStorage` 和 `FlashParamStorage` 三层组成。
业务模块只使用名称和类型访问 `ParamManager`，不会依赖 STM32 Flash；存储后端可以
在其他机器人上替换为 FRAM、EEPROM 或文件实现。

当前注册 142 个参数，包括 RC 校准与映射、控制分配系数以及 CAN 执行器协议参数。
参数名不超过 16 字符，为后续 MAVLink `PARAM_ID` 兼容预留。转向执行器使用
`PMIN/PMAX` 限制机械指令位置，使用独立的 `PFBMAX` 配置达妙反馈协议量程。

STM32F407 的扇区 10 和 11 分别位于 `0x080c0000` 和 `0x080e0000`。链接脚本将
固件区限制为前 768 KiB。保存时擦除非活动扇区，写入页头、参数记录和 CRC，最后
单独写入提交标记。掉电导致的新页不完整时，启动会继续加载另一页。

修改参数会立即发布 `parameter_update`，`rc_update` 随即刷新标定值和通道映射。
`robot_core` 每 100 ms 检查一次自动保存条件；最后一次修改后静默一秒才保存，
用于合并 QGC 后续连续下发的参数。

NSH 验收命令：

```sh
param status
param show RC1_*
param set RC1_MIN 1045
param set RC_MAP_ROLL 1
param save
param get RC1_MIN
rc_update status
```
