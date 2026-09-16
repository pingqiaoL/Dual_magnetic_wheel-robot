# params

本目录实现参数声明、类型校验、运行时修改、`parameter_update` 通知和 Flash
自动持久化。`ParamManager` 与介质无关，`FlashParamStorage` 是 CBoard 使用的
A/B 扇区后端，具体设计与 NSH 命令见 `docs/step5-parameters.md`。
