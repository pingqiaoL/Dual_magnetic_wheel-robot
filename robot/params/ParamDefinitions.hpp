/**
 * @file ParamDefinitions.hpp
 * @brief 声明集中参数定义表的构造入口。
 */

#pragma once

class ParamManager;

/** 将全部参数名称、类型和固件默认值注册到管理器。 */
void buildParamDefinitions(ParamManager &manager);
