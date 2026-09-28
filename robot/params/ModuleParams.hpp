/**
 * @file ModuleParams.hpp
 * @brief 定义 PX4 风格的轻量参数父子树。
 */

#pragma once

class ParamBase;

/**
 * 一个模块作为参数树根，它拥有的组件在构造时传入父节点。
 * updateParams() 会更新本节点参数并递归更新所有子节点。
 */
class ModuleParams
{
public:
  explicit ModuleParams(ModuleParams *parent = nullptr);
  virtual ~ModuleParams();

  ModuleParams(const ModuleParams &) = delete;
  ModuleParams &operator=(const ModuleParams &) = delete;

  /** 更新本节点注册参数、定制参数组以及全部子节点。 */
  bool updateParams();

  /** 工厂先创建子对象时，可在所有者构造后再接入参数树。 */
  void setParent(ModuleParams *parent);

protected:
  /** 数组形式的参数组可重写此函数，普通 ParamInt/Float 不需要。 */
  virtual bool updateParamsImpl() { return true; }

private:
  friend class ParamBase;
  void registerParam(ParamBase *parameter);
  void unregisterParam(ParamBase *parameter);
  void registerChild(ModuleParams *child);
  void unregisterChild(ModuleParams *child);

  ModuleParams *_parent{nullptr};
  ModuleParams *_firstChild{nullptr};
  ModuleParams *_nextSibling{nullptr};
  ParamBase *_firstParam{nullptr};
};
