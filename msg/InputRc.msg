# SBUS 驱动发布给 rc_update 的原始遥控输入。

uint8 MAX_CHANNELS = 16
uint8 SOURCE_UNKNOWN = 0
uint8 SOURCE_SBUS = 1

uint64 timestamp                    # 消息发布时间，单位微秒
uint64 timestampLastSignal          # 最近完整 SBUS 帧时间，单位微秒
uint16[16] values                   # 16 路原始通道脉宽值
uint32 lostFrameCount               # 累计丢帧数量
uint32 totalFrameCount              # 累计有效帧数量
int16 rssi                          # 信号强度，-1 表示接收机未提供
uint8 channelCount                  # 当前有效通道数量
uint8 inputSource                   # 输入协议来源
bool rcFailsafe                     # 接收机明确报告 failsafe
bool rcLost                         # 超时未收到完整帧
bool frameLost                      # 当前帧带有丢帧标志
