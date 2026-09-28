# command发布的统一执行器安全状态；输出驱动不得自行解读遥控开关。
uint64 timestamp                # 发布安全状态的时间，微秒
bool armed                      # 已完成上电OFF检查且当前开关为ON
bool valid                      # 当前遥控输入及开关映射有效；丢失不自动清除armed
bool lockdown                   # NSH手动禁止输出
