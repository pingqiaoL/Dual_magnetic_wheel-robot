/**
 * @file Param.hpp
 * @brief 定义使用缓存句柄读取参数的 int32 和 float 参数对象。
 */

#pragma once

#include "robot/params/ModuleParams.hpp"
#include "robot/params/ParamManager.hpp"

/** 参数对象公共基类；构造时自动注册到所属 ModuleParams 节点。 */
class ParamBase
{
public:
  ParamBase(ModuleParams *owner, const char *name);
  virtual ~ParamBase();

  ParamBase(const ParamBase &) = delete;
  ParamBase &operator=(const ParamBase &) = delete;

  /** 使用缓存句柄刷新参数值；句柄尚未找到时会再次查找。 */
  virtual bool update() = 0;

protected:
  bool ensureHandle();
  ParamHandle handle() const { return _handle; }

private:
  friend class ModuleParams;
  ModuleParams *_owner;
  const char *_name;
  ParamHandle _handle{ParamInvalid};
  ParamBase *_next{nullptr};
};

/** 缓存一个 int32 参数。 */
class ParamInt final : public ParamBase
{
public:
  ParamInt(ModuleParams *owner, const char *name, int32_t fallback = 0)
      : ParamBase(owner, name), _value(fallback) { (void)update(); }

  bool update() override;
  int32_t get() const { return _value; }
  operator int32_t() const { return _value; }

private:
  int32_t _value;
};

/** 缓存一个 float 参数。 */
class ParamFloat final : public ParamBase
{
public:
  ParamFloat(ModuleParams *owner, const char *name, float fallback = 0.0f)
      : ParamBase(owner, name), _value(fallback) { (void)update(); }

  bool update() override;
  float get() const { return _value; }
  operator float() const { return _value; }

private:
  float _value;
};
