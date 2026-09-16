/**
 * @file ParamManager.hpp
 * @brief 声明 PX4 风格的参数查找、读写、通知和自动保存管理器。
 */

#pragma once

#include "robot/os/Mutex.hpp"
#include "robot/params/ParamStorage.hpp"

#include <stddef.h>
#include <stdint.h>

/** 集中管理所有可调参数，并向业务模块发布参数变化。 */
class ParamManager
{
public:
  /** 返回进程内唯一的参数管理器。 */
  static ParamManager &instance();

  /** 注册默认参数并从指定后端加载已保存值；重复调用安全。 */
  bool initialize(ParamStorage &storage);

  /** 按名称查找参数，失败返回 ParamInvalid。 */
  ParamHandle find(const char *name) const;

  /** 返回参数定义数量。 */
  size_t count() const;

  /** 返回指定参数定义；句柄无效时返回 nullptr。 */
  const ParamDefinition *definition(ParamHandle handle) const;

  /** 读取 int32 参数。 */
  bool get(ParamHandle handle, int32_t &value) const;

  /** 读取 float 参数。 */
  bool get(ParamHandle handle, float &value) const;

  /** 按名称读取 int32 参数。 */
  bool get(const char *name, int32_t &value) const;

  /** 按名称读取 float 参数。 */
  bool get(const char *name, float &value) const;

  /** 修改 int32 参数并发布 parameter_update。 */
  bool set(ParamHandle handle, int32_t value);

  /** 修改 float 参数并发布 parameter_update。 */
  bool set(ParamHandle handle, float value);

  /** 修改 int32 构型默认值；仅当当前值仍是旧默认值时同步当前值。 */
  bool setDefault(ParamHandle handle, int32_t value);

  /** 修改 float 构型默认值；保留用户已经覆盖的当前值。 */
  bool setDefault(ParamHandle handle, float value);

  /** 恢复单个参数的默认值。 */
  bool reset(ParamHandle handle);

  /** 恢复全部参数默认值。 */
  void resetAll();

  /** 立即把当前非默认参数写入持久化后端。 */
  bool save();

  /** 从持久化后端重新加载参数。 */
  bool load();

  /** 由低频任务调用，参数停止修改一秒后自动保存一次。 */
  void pollAutoSave(uint64_t nowMicroseconds);

  /** 查询当前参数是否存在未保存修改。 */
  bool dirty() const;

  /** 返回参数修改代数。 */
  uint32_t generation() const;

  /** 返回最近一次成功保存的存储序号。 */
  uint32_t storageSequence() const;

private:
  ParamManager();
  ParamManager(const ParamManager &) = delete;
  ParamManager &operator=(const ParamManager &) = delete;

  /** 构造本项目首批 RC 参数定义。 */
  void buildDefinitions();

  /** 注册一个 int32 参数。 */
  void addInt(const char *name, int32_t defaultValue);

  /** 注册一个 float 参数。 */
  void addFloat(const char *name, float defaultValue);

  /** 将指定原始值写入内存，并完成修改标记。 */
  bool setRaw(ParamHandle handle, ParamType type, uint32_t rawValue);

  /** 修改参数定义中的默认值，并按需更新尚未被用户覆盖的当前值。 */
  bool setDefaultRaw(ParamHandle handle, ParamType type, uint32_t rawValue);

  /** 在不持锁的情况下发布参数更新通知。 */
  void publishUpdate(uint32_t generation, bool saved);

  /** 把当前非默认参数复制到存储快照。 */
  void makeSnapshot(ParamStorageSnapshot &snapshot,
                    uint32_t sequence) const;

  /** 将已校验的存储快照合并到默认参数上。 */
  void applySnapshot(const ParamStorageSnapshot &snapshot);

  static constexpr uint64_t AutoSaveDelayMicroseconds = 1000000ULL;

  mutable os::Mutex _mutex;
  ParamStorage *_storage;
  ParamDefinition _definitions[ParamMaxCount];
  ParamValue _values[ParamMaxCount];
  size_t _count;
  uint32_t _generation;
  uint32_t _storageSequence;
  uint64_t _lastChangeTimestamp;
  bool _initialized;
  bool _dirty;
};
