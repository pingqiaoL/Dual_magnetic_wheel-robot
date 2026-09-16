# CAN 输出层发布的执行器反馈和链路状态。

uint8 MAX_ACTUATORS = 16
uint64 timestamp                    # 最近一次状态发布时间
uint64[16] timestampSample          # 每个执行器最近反馈时间
float32[16] position                # 电机位置，rad
float32[16] velocity                # 电机速度，rad/s
float32[16] torque                  # 电机估算转矩，N*m
float32[16] mosTemperature          # MOS 温度，摄氏度
float32[16] coilTemperature         # 线圈温度，摄氏度
uint8[16] error                     # 达妙状态码
bool[16] online                     # 最近 500 ms 内收到反馈
uint8 count                         # 当前构型执行器总数
uint32 txCount                      # 成功发送的 CAN 帧数
uint32 rxCount                      # 成功解析的反馈帧数
uint32 errorCount                   # CAN 读写或协议错误数
