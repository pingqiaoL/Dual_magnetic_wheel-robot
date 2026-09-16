# orb

这里实现面向当前单片机项目的轻量发布/订阅总线。每个 `Topic<T>` 只保存
最新一份消息和递增的 generation；发布者与订阅者通过 topic 交换副本，
不直接调用彼此。数据复制由互斥锁保护，topic 运行期间不动态分配内存。

当前 topic 包括 `input_rc`、`rc_channels`、`manual_control` 和
`manual_control_switches`，其消息数据结构统一放在项目根目录 `msg/`，
topic 唯一实例在 `Topics.cpp` 中创建。任务以普通线程轮询订阅，不依赖
PX4 任务队列。
