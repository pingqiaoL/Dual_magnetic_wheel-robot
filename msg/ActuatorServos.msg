# ControlAllocator 发布的转向舵机归一化输出。

uint8 MAX_CONTROLS = 8
uint64 timestamp                    # 消息发布时间，单位微秒
uint64 timestampSample              # 对应的遥控输入时间
float32[8] control                  # 转向输出，范围 -1 到 1
uint8 count                         # 当前构型使用的转向执行器数量
bool valid                          # 控制输入和分配结果有效
