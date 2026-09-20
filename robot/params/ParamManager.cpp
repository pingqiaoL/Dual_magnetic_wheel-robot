/**
 * @file ParamManager.cpp
 * @brief 实现参数表、类型安全访问、uORB 通知与延迟自动保存。
 */

#include "robot/params/ParamManager.hpp"

#include "robot/orb/Topics.hpp"
#include "robot/os/Clock.hpp"

#include <math.h>
#include <stdio.h>
#include <string.h>

/** 比较参数原始值，避免浮点比较带来的特殊情况。 */
bool ParamManager::sameValue(const ParamValue &left, const ParamValue &right)
{
  return left.raw == right.raw;
}

/** 返回静态构造的参数管理器。 */
ParamManager &ParamManager::instance()
{
  static ParamManager manager;
  return manager;
}

/** 创建默认参数表，尚不访问硬件存储。 */
ParamManager::ParamManager()
    : _storage(nullptr),
      _count(0),
      _generation(0),
      _storageSequence(0),
      _lastChangeTimestamp(0),
      _initialized(false),
      _dirty(false)
{
  memset(_definitions, 0, sizeof(_definitions));
  memset(_values, 0, sizeof(_values));
  buildDefinitions();
}

/** 连接存储后端，并在存在有效快照时恢复已保存参数。 */
bool ParamManager::initialize(ParamStorage &storage)
{
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked())
      {
        return false;
      }

    if (_initialized)
      {
        return true;
      }

    _storage = &storage;
    _initialized = true;
  }

  ParamStorageSnapshot snapshot{};
  if (storage.load(snapshot))
    {
      os::LockGuard guard(_mutex);
      if (!guard.locked())
        {
          return false;
        }

      applySnapshot(snapshot);
      _storageSequence = snapshot.sequence;
    }

  return true;
}

/** 查找名称完全匹配的参数。 */
ParamHandle ParamManager::find(const char *name) const
{
  if (name == nullptr)
    {
      return ParamInvalid;
    }

  os::LockGuard guard(_mutex);
  if (!guard.locked())
    {
      return ParamInvalid;
    }

  for (size_t index = 0; index < _count; ++index)
    {
      if (strcmp(_definitions[index].name, name) == 0)
        {
          return static_cast<ParamHandle>(index);
        }
    }

  return ParamInvalid;
}

/** 返回当前注册参数数量。 */
size_t ParamManager::count() const
{
  os::LockGuard guard(_mutex);
  return guard.locked() ? _count : 0;
}

/** 返回只读参数定义。定义表初始化后不再移动。 */
const ParamDefinition *ParamManager::definition(ParamHandle handle) const
{
  os::LockGuard guard(_mutex);
  if (!guard.locked() || handle >= _count)
    {
      return nullptr;
    }

  return &_definitions[handle];
}

/** 读取类型为 int32 的参数。 */
bool ParamManager::get(ParamHandle handle, int32_t &value) const
{
  os::LockGuard guard(_mutex);
  if (!guard.locked() || handle >= _count ||
      _definitions[handle].type != ParamType::Int32)
    {
      return false;
    }

  value = _values[handle].integer;
  return true;
}

/** 读取类型为 float 的参数。 */
bool ParamManager::get(ParamHandle handle, float &value) const
{
  os::LockGuard guard(_mutex);
  if (!guard.locked() || handle >= _count ||
      _definitions[handle].type != ParamType::Float)
    {
      return false;
    }

  value = _values[handle].real;
  return true;
}

/** 按名称读取 int32 参数。 */
bool ParamManager::get(const char *name, int32_t &value) const
{
  return get(find(name), value);
}

/** 按名称读取 float 参数。 */
bool ParamManager::get(const char *name, float &value) const
{
  return get(find(name), value);
}

/** 修改 int32 参数。 */
bool ParamManager::set(ParamHandle handle, int32_t value)
{
  ParamValue converted{};
  converted.integer = value;
  return setRaw(handle, ParamType::Int32, converted.raw);
}

/** 修改有限的 float 参数，拒绝 NaN 和无穷值。 */
bool ParamManager::set(ParamHandle handle, float value)
{
  if (!isfinite(value))
    {
      return false;
    }

  ParamValue converted{};
  converted.real = value;
  return setRaw(handle, ParamType::Float, converted.raw);
}

/** 设置 int32 构型默认值，不把该操作标记为用户修改。 */
bool ParamManager::setDefault(ParamHandle handle, int32_t value)
{
  ParamValue converted{};
  converted.integer = value;
  return setDefaultRaw(handle, ParamType::Int32, converted.raw);
}

/** 设置有限的 float 构型默认值，不覆盖已经加载的用户值。 */
bool ParamManager::setDefault(ParamHandle handle, float value)
{
  if (!isfinite(value))
    {
      return false;
    }

  ParamValue converted{};
  converted.real = value;
  return setDefaultRaw(handle, ParamType::Float, converted.raw);
}

/** 恢复一个参数的默认值并发出一次更新。 */
bool ParamManager::reset(ParamHandle handle)
{
  ParamType type;
  uint32_t raw;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked() || handle >= _count)
      {
        return false;
      }

    type = _definitions[handle].type;
    raw = _definitions[handle].defaultValue.raw;
  }

  return setRaw(handle, type, raw);
}

/** 一次性恢复全部参数，避免为每个参数发布通知。 */
void ParamManager::resetAll()
{
  uint32_t changedGeneration = 0;
  bool changed = false;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked())
      {
        return;
      }

    for (size_t index = 0; index < _count; ++index)
      {
        if (!sameValue(_values[index], _definitions[index].defaultValue))
          {
            _values[index] = _definitions[index].defaultValue;
            changed = true;
          }
      }

    if (changed)
      {
        _dirty = true;
        _lastChangeTimestamp = os::Clock::nowMicroseconds();
        changedGeneration = ++_generation;
      }
  }

  if (changed)
    {
      publishUpdate(changedGeneration, false);
    }
}

/** 保存参数快照，并仅在保存期间没有新修改时清除 dirty。 */
bool ParamManager::save()
{
  ParamStorageSnapshot snapshot{};
  ParamStorage *storage = nullptr;
  uint32_t capturedGeneration = 0;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked() || !_initialized || _storage == nullptr)
      {
        return false;
      }

    if (!_dirty)
      {
        return true;
      }

    storage = _storage;
    capturedGeneration = _generation;
    makeSnapshot(snapshot, _storageSequence + 1U);
  }

  if (!storage->save(snapshot))
    {
      return false;
    }

  bool allSaved = false;
  uint32_t currentGeneration = 0;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked())
      {
        return false;
      }

    _storageSequence = snapshot.sequence;
    allSaved = _generation == capturedGeneration;
    if (allSaved)
      {
        _dirty = false;
      }
    currentGeneration = _generation;
  }

  publishUpdate(currentGeneration, allSaved);
  return true;
}

/** 重新加载最后一份有效快照，并通知所有参数使用者。 */
bool ParamManager::load()
{
  ParamStorage *storage = nullptr;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked() || !_initialized || _storage == nullptr)
      {
        return false;
      }
    storage = _storage;
  }

  ParamStorageSnapshot snapshot{};
  if (!storage->load(snapshot))
    {
      return false;
    }

  uint32_t changedGeneration = 0;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked())
      {
        return false;
      }

    for (size_t index = 0; index < _count; ++index)
      {
        _values[index] = _definitions[index].defaultValue;
      }
    applySnapshot(snapshot);
    _storageSequence = snapshot.sequence;
    _dirty = false;
    changedGeneration = ++_generation;
  }

  publishUpdate(changedGeneration, true);
  return true;
}

/** 参数停止变化达到延迟时间后保存，合并 QGC 连续写入。 */
void ParamManager::pollAutoSave(uint64_t nowMicroseconds)
{
  bool due = false;
  {
    os::LockGuard guard(_mutex);
    due = guard.locked() && _dirty && _lastChangeTimestamp != 0 &&
          nowMicroseconds >= _lastChangeTimestamp &&
          nowMicroseconds - _lastChangeTimestamp >=
              AutoSaveDelayMicroseconds;
  }

  if (due)
    {
      (void)save();
    }
}

/** 查询是否有未保存值。 */
bool ParamManager::dirty() const
{
  os::LockGuard guard(_mutex);
  return guard.locked() && _dirty;
}

/** 返回运行时修改代数。 */
uint32_t ParamManager::generation() const
{
  os::LockGuard guard(_mutex);
  return guard.locked() ? _generation : 0;
}

/** 返回 Flash 快照序号。 */
uint32_t ParamManager::storageSequence() const
{
  os::LockGuard guard(_mutex);
  return guard.locked() ? _storageSequence : 0;
}

/** 注册 16 路校准参数、主控制映射、12 路开关映射和通道数。 */
void ParamManager::buildDefinitions()
{
  char name[ParamNameLength];
  for (unsigned channel = 1; channel <= 16; ++channel)
    {
      snprintf(name, sizeof(name), "RC%u_MIN", channel);
      addFloat(name, 1000.0f);
      snprintf(name, sizeof(name), "RC%u_MAX", channel);
      addFloat(name, 2000.0f);
      snprintf(name, sizeof(name), "RC%u_TRIM", channel);
      addFloat(name, 1500.0f);
      snprintf(name, sizeof(name), "RC%u_DZ", channel);
      addFloat(name, 20.0f);
      snprintf(name, sizeof(name), "RC%u_REV", channel);
      addFloat(name, 1.0f);
    }

  addInt("RC_MAP_THROTTLE", 3);
  addInt("RC_MAP_ROLL", 1);
  addInt("RC_MAP_PITCH", 2);
  addInt("RC_MAP_YAW", 4);

  for (unsigned switchIndex = 1; switchIndex <= 12; ++switchIndex)
    {
      snprintf(name, sizeof(name), "RC_MAP_SW%u", switchIndex);
      addInt(name, static_cast<int32_t>(switchIndex + 4U));
    }

  addInt("RC_CHAN_CNT", 16);

  addInt("SYS_AUTOSTART", 1);
  addInt("CA_AIRFRAME", 1);
  addFloat("CA_THR_F", 1.0f);
  addFloat("CA_THR_R", 1.0f);
  addFloat("CA_YAW_F", 1.0f);
  addFloat("CA_YAW_R", 0.0f);
  addInt("CA_ARM_SW", 1);

  for (unsigned index = 0; index < 2; ++index)
    {
      snprintf(name, sizeof(name), "CAN_M%u_PROTO", index);
      addInt(name, 1);
      snprintf(name, sizeof(name), "CAN_M%u_TYPE", index);
      addInt(name, 2);
      snprintf(name, sizeof(name), "CAN_M%u_ID", index);
      addInt(name, static_cast<int32_t>(index + 1U));
      snprintf(name, sizeof(name), "CAN_M%u_FBID", index);
      addInt(name, 0);
      snprintf(name, sizeof(name), "CAN_M%u_MODE", index);
      addInt(name, 3);
      snprintf(name, sizeof(name), "CAN_M%u_REV", index);
      addFloat(name, 1.0f);
      snprintf(name, sizeof(name), "CAN_M%u_PMAX", index);
      addFloat(name, 12.5f);
      snprintf(name, sizeof(name), "CAN_M%u_VMAX", index);
      addFloat(name, 30.0f);
      snprintf(name, sizeof(name), "CAN_M%u_TMAX", index);
      addFloat(name, 10.0f);
    }

  for (unsigned index = 0; index < 2; ++index)
    {
      snprintf(name, sizeof(name), "CAN_S%u_PROTO", index);
      addInt(name, 1);
      snprintf(name, sizeof(name), "CAN_S%u_TYPE", index);
      addInt(name, 1);
      snprintf(name, sizeof(name), "CAN_S%u_ID", index);
      addInt(name, static_cast<int32_t>(index + 3U));
      snprintf(name, sizeof(name), "CAN_S%u_FBID", index);
      addInt(name, 0);
      snprintf(name, sizeof(name), "CAN_S%u_MODE", index);
      addInt(name, 2);
      snprintf(name, sizeof(name), "CAN_S%u_REV", index);
      addFloat(name, 1.0f);
      snprintf(name, sizeof(name), "CAN_S%u_PMIN", index);
      addFloat(name, -1.0f);
      snprintf(name, sizeof(name), "CAN_S%u_PMAX", index);
      addFloat(name, 1.0f);
      snprintf(name, sizeof(name), "CAN_S%u_PFBMAX", index);
      addFloat(name, 12.5f);
      snprintf(name, sizeof(name), "CAN_S%u_VMAX", index);
      addFloat(name, 5.0f);
      snprintf(name, sizeof(name), "CAN_S%u_TMAX", index);
      addFloat(name, 10.0f);
    }
}

/** 向参数表末尾添加 int32 默认值。 */
void ParamManager::addInt(const char *name, int32_t defaultValue)
{
  if (_count >= ParamMaxCount)
    {
      return;
    }

  snprintf(_definitions[_count].name, ParamNameLength, "%s", name);
  _definitions[_count].type = ParamType::Int32;
  _definitions[_count].defaultValue.integer = defaultValue;
  _values[_count] = _definitions[_count].defaultValue;
  ++_count;
}

/** 向参数表末尾添加 float 默认值。 */
void ParamManager::addFloat(const char *name, float defaultValue)
{
  if (_count >= ParamMaxCount)
    {
      return;
    }

  snprintf(_definitions[_count].name, ParamNameLength, "%s", name);
  _definitions[_count].type = ParamType::Float;
  _definitions[_count].defaultValue.real = defaultValue;
  _values[_count] = _definitions[_count].defaultValue;
  ++_count;
}

/** 完成类型校验、值更新、dirty 标记和单次通知。 */
bool ParamManager::setRaw(ParamHandle handle, ParamType type,
                          uint32_t rawValue)
{
  uint32_t changedGeneration = 0;
  bool changed = false;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked() || handle >= _count ||
        _definitions[handle].type != type)
      {
        return false;
      }

    if (_values[handle].raw != rawValue)
      {
        _values[handle].raw = rawValue;
        _dirty = true;
        _lastChangeTimestamp = os::Clock::nowMicroseconds();
        changedGeneration = ++_generation;
        changed = true;
      }
  }

  if (changed)
    {
      publishUpdate(changedGeneration, false);
    }
  return true;
}

/** 改写默认值；当前值等于旧默认值表示用户没有覆盖，随默认值一起更新。 */
bool ParamManager::setDefaultRaw(ParamHandle handle, ParamType type,
                                 uint32_t rawValue)
{
  uint32_t changedGeneration = 0;
  bool currentChanged = false;
  {
    os::LockGuard guard(_mutex);
    if (!guard.locked() || handle >= _count ||
        _definitions[handle].type != type)
      {
        return false;
      }

    const uint32_t oldDefault = _definitions[handle].defaultValue.raw;
    if (oldDefault == rawValue)
      {
        return true;
      }

    if (_values[handle].raw == oldDefault)
      {
        _values[handle].raw = rawValue;
        currentChanged = true;
      }
    _definitions[handle].defaultValue.raw = rawValue;
    changedGeneration = ++_generation;
  }

  publishUpdate(changedGeneration, false);
  (void)currentChanged;
  return true;
}

/** 发布最新参数代数和保存状态。 */
void ParamManager::publishUpdate(uint32_t generation, bool saved)
{
  ParameterUpdate update{};
  update.timestamp = os::Clock::nowMicroseconds();
  update.generation = generation;
  update.saved = saved;
  parameterUpdateTopic().publish(update);
}

/** 只保存偏离默认值的参数，减少 Flash 写入字节数。 */
void ParamManager::makeSnapshot(ParamStorageSnapshot &snapshot,
                                uint32_t sequence) const
{
  memset(&snapshot, 0, sizeof(snapshot));
  snapshot.sequence = sequence;

  for (size_t index = 0;
       index < _count && snapshot.count < ParamMaxCount; ++index)
    {
      if (sameValue(_values[index], _definitions[index].defaultValue))
        {
          continue;
        }

      ParamStoredRecord &record = snapshot.records[snapshot.count++];
      snprintf(record.name, sizeof(record.name), "%s",
               _definitions[index].name);
      record.type = static_cast<uint8_t>(_definitions[index].type);
      record.valueRaw = _values[index].raw;
    }
}

/** 按名称和类型恢复参数，自动忽略新固件无法识别的旧记录。 */
void ParamManager::applySnapshot(const ParamStorageSnapshot &snapshot)
{
  const size_t recordCount = snapshot.count < ParamMaxCount
                                 ? snapshot.count
                                 : ParamMaxCount;
  for (size_t recordIndex = 0; recordIndex < recordCount; ++recordIndex)
    {
      const ParamStoredRecord &record = snapshot.records[recordIndex];
      for (size_t parameterIndex = 0; parameterIndex < _count;
           ++parameterIndex)
        {
          if (strncmp(_definitions[parameterIndex].name, record.name,
                      ParamNameLength) == 0 &&
              static_cast<uint8_t>(_definitions[parameterIndex].type) ==
                  record.type)
            {
              _values[parameterIndex].raw = record.valueRaw;
              break;
            }
        }
    }
}
