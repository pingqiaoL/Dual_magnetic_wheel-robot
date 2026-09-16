/**
 * @file ParamStorage.hpp
 * @brief 声明可替换的参数持久化接口，便于使用 Flash 或主机测试存储。
 */

#pragma once

#include "robot/params/ParamTypes.hpp"

/** 参数持久化后端接口。 */
class ParamStorage
{
public:
  virtual ~ParamStorage() = default;

  /** 从持久化介质读取最后一份完整快照。 */
  virtual bool load(ParamStorageSnapshot &snapshot) = 0;

  /** 原子地保存一份快照。 */
  virtual bool save(const ParamStorageSnapshot &snapshot) = 0;
};
