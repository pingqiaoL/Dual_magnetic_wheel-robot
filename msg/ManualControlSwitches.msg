# rc_update 输出给上层控制模块的十二路开关状态。

uint8 POSITION_NONE = 0
uint8 POSITION_ON = 1
uint8 POSITION_MIDDLE = 2
uint8 POSITION_OFF = 3
uint8 SWITCH_COUNT = 12

uint64 timestamp                    # 消息发布时间，单位微秒
uint64 timestampSample              # 对应原始 SBUS 帧时间
uint8[12] positions                 # 十二路开关位置
uint8 switchCount                   # 实际发布的开关数量
bool valid                          # 三帧稳定且链路有效时为 true
