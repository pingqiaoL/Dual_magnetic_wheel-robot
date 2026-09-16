# uORB 消息定义

本目录统一保存下位机全部 uORB 消息的数据定义。和 PX4 一样，开发者只在
`.msg` 文件中写字段和常量；`tools/generate_messages.py` 在构建前生成 C++
结构体头文件，生成结果位于 `build/generated/msg/`，不得手工修改。

当前消息：

- `InputRc.msg`：SBUS 驱动发布的原始遥控通道和链路状态。
- `RcChannels.msg`：经过校准和归一化的通道及功能映射。
- `ManualControl.msg`：上层使用的四轴手动输入。
- `ManualControlSwitches.msg`：上层使用的十二路开关状态。

新增 topic 时，只需在这里增加 `.msg` 文件，再在 `robot/orb/Topics.hpp`
和 `Topics.cpp` 中声明并创建对应的唯一 topic 实例。
