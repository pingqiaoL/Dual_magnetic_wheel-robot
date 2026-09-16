# rc_update 输出给上层控制模块的四轴手动输入。

uint64 timestamp                    # 消息发布时间，单位微秒
uint64 timestampSample              # 对应原始 SBUS 帧时间
float32 roll                        # 横滚输入，范围为 -1 到 1
float32 pitch                       # 俯仰输入，范围为 -1 到 1
float32 yaw                         # 偏航输入，范围为 -1 到 1
float32 throttle                    # 油门输入，范围为 -1 到 1
bool valid                          # 三帧稳定且链路有效时为 true
