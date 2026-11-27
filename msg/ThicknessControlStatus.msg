# 测厚机构控制状态，供下位机模块订阅。
uint8 WAITING_FOR_ARM = 0
uint8 HOMING = 1
uint8 WAITING_FOR_FIRST_WORK = 2
uint8 MANUAL_CONTROL = 3
uint8 FAULT = 4

uint64 timestamp          # 单调时钟，单位微秒
uint8 phase               # 本消息定义的控制阶段
uint8 sfPosition          # ManualControlSwitches::POSITION_*，ON=上，OFF=下
uint8 liftStatus          # ProbeLift::Status
uint8 liftError           # ProbeLift::Error
uint64 readySince         # 当前就绪周期起点，未就绪时为 0
bool measurementReady     # 有效控制期间下压到位且工作开关稳定
