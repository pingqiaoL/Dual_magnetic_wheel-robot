# RobotControl输出给控制分配器的机器人运动目标。
uint64 timestamp                    # 消息发布时间，微秒
uint64 timestampSample              # 最近一次有效遥控采样时间，微秒
float32 speed                       # 归一化行驶速度，范围-1到1
float32 steering                    # 归一化转向目标，范围-1到1
bool valid                          # 已取得有效目标，可进入控制分配
bool failsafe                       # 当前目标已按Failsafe状态应用安全策略
