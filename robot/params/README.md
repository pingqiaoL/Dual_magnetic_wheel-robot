# params

本目录实现参数声明、类型校验、运行时修改、`parameter_update` 通知和 Flash
自动持久化。`ParamManager` 与介质无关，`FlashParamStorage` 是 CBoard 使用的
A/B 扇区后端，具体设计与 NSH 命令见 `docs/step5-parameters.md`。

`ParamDefinitions.cpp` 使用 `PARAM_DEFINE_INT32/FLOAT` 集中注册名称、类型和
固件默认值。`ParamInt`、`ParamFloat` 在对象构造时缓存句柄；`ModuleParams`
把模块及其组件连接成无动态分配的父子树。模块收到 `parameter_update` 后
调用一次根节点 `updateParams()`，更新会递归传递到 `RcConfig`、构型模型、
`MixingOutput` 等子节点。16 路 RC 校准仍使用句柄数组，不创建大量子对象。
