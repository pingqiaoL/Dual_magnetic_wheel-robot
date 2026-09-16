/**
 * @file ParamSystem.hpp
 * @brief 提供板级参数后端与可复用 ParamManager 之间的唯一装配入口。
 */

#pragma once

#include "robot/params/ParamManager.hpp"

/** 初始化 CBoard Flash 参数存储；重复调用安全。 */
bool paramSystemInitialize();

/** 返回已经注册默认表的全局参数管理器。 */
ParamManager &params();
