# 参数系统在运行时发生修改或完成持久化时发布。

uint64 timestamp        # 通知发布时间，单位微秒
uint32 generation       # 每次参数修改后递增
bool saved              # 当前内存参数是否已经写入持久化存储
