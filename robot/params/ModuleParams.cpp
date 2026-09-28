/** @file ModuleParams.cpp
 * @brief 实现无动态分配的参数父子链表和缓存参数对象。
 */

#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"

ModuleParams::ModuleParams(ModuleParams *parent) : _parent(parent)
{
  if (_parent != nullptr) { _parent->registerChild(this); }
}

ModuleParams::~ModuleParams()
{
  if (_parent != nullptr) { _parent->unregisterChild(this); }
}

void ModuleParams::setParent(ModuleParams *parent)
{
  if (_parent == parent) { return; }
  if (_parent != nullptr) { _parent->unregisterChild(this); }
  _parent = parent;
  if (_parent != nullptr) { _parent->registerChild(this); }
}

void ModuleParams::registerParam(ParamBase *parameter)
{
  if (parameter == nullptr) { return; }
  parameter->_next = _firstParam;
  _firstParam = parameter;
}

void ModuleParams::unregisterParam(ParamBase *parameter)
{
  ParamBase **current = &_firstParam;
  while (*current != nullptr)
    {
      if (*current == parameter)
        {
          *current = parameter->_next;
          parameter->_next = nullptr;
          return;
        }
      current = &((*current)->_next);
    }
}

void ModuleParams::registerChild(ModuleParams *child)
{
  if (child == nullptr) { return; }
  child->_nextSibling = _firstChild;
  _firstChild = child;
}

void ModuleParams::unregisterChild(ModuleParams *child)
{
  ModuleParams **current = &_firstChild;
  while (*current != nullptr)
    {
      if (*current == child)
        {
          *current = child->_nextSibling;
          child->_nextSibling = nullptr;
          return;
        }
      current = &((*current)->_nextSibling);
    }
}

bool ModuleParams::updateParams()
{
  bool valid = true;
  for (ParamBase *parameter = _firstParam; parameter != nullptr;
       parameter = parameter->_next)
    {
      valid = parameter->update() && valid;
    }
  valid = updateParamsImpl() && valid;
  for (ModuleParams *child = _firstChild; child != nullptr;
       child = child->_nextSibling)
    {
      valid = child->updateParams() && valid;
    }
  return valid;
}

ParamBase::ParamBase(ModuleParams *owner, const char *name)
    : _owner(owner), _name(name)
{
  if (_owner != nullptr) { _owner->registerParam(this); }
}

ParamBase::~ParamBase()
{
  if (_owner != nullptr) { _owner->unregisterParam(this); }
}

bool ParamBase::ensureHandle()
{
  if (_handle == ParamInvalid && _name != nullptr)
    { _handle = ParamManager::instance().find(_name); }
  return _handle != ParamInvalid;
}

bool ParamInt::update()
{
  return ensureHandle() && ParamManager::instance().get(handle(), _value);
}

bool ParamFloat::update()
{
  return ensureHandle() && ParamManager::instance().get(handle(), _value);
}
