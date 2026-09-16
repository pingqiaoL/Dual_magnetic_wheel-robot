# rc_update 发布的已校准、归一化遥控通道。

uint8 FUNCTION_THROTTLE = 0
uint8 FUNCTION_ROLL = 1
uint8 FUNCTION_PITCH = 2
uint8 FUNCTION_YAW = 3
uint8 FUNCTION_SWITCH_FIRST = 4
uint8 SWITCH_COUNT = 12
uint8 FUNCTION_COUNT = 16

uint64 timestamp                    # 消息发布时间，单位微秒
uint64 timestampLastValid           # 最近有效遥控信号时间
float32[16] channels                # 归一化通道，范围为 -1 到 1
int8[16] function                   # 功能编号到物理通道编号的映射
uint32 frameDropCount               # 累计丢帧数量
uint8 channelCount                  # 当前有效通道数量
bool signalLost                     # 遥控链路是否失效
