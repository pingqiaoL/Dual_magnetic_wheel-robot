# ControlAllocator 发布的动力电机归一化输出。

uint8 MAX_CONTROLS = 8
uint64 timestamp                    # 消息发布时间，单位微秒
uint64 timestampSample              # 对应的遥控输入时间
float32[8] control                  # 动力输出，范围 -1 到 1
uint8 count                         # 当前构型使用的动力电机数量
bool valid                          # 控制输入和分配结果有效
