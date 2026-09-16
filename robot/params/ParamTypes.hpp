/**
 * @file ParamTypes.hpp
 * @brief 定义参数类型、句柄以及与存储层交换的稳定数据格式。
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

/** 参数名称最多使用 16 个字符，以兼容 MAVLink PARAM_ID。 */
static constexpr size_t ParamNameLength = 17;

/** 参数系统最多注册 128 个参数。 */
static constexpr size_t ParamMaxCount = 160;

/** 无效参数句柄。 */
using ParamHandle = uint16_t;
static constexpr ParamHandle ParamInvalid = UINT16_MAX;

/** 参数支持的基础类型。 */
enum class ParamType : uint8_t
{
  Int32 = 1,
  Float = 2
};

/** 在内存中保存一个 int32 或 float 参数值。 */
union ParamValue
{
  int32_t integer;
  float real;
  uint32_t raw;
};

/** 描述一个参数的名称、类型和默认值。 */
struct ParamDefinition
{
  char name[ParamNameLength];
  ParamType type;
  ParamValue defaultValue;
};

/** Flash 中按名称保存的参数记录，允许固件后续增加参数而不破坏旧数据。 */
struct ParamStoredRecord
{
  char name[ParamNameLength];
  uint8_t type;
  uint8_t reserved[2];
  uint32_t valueRaw;
};

/** 参数存储层一次加载或保存的快照。 */
struct ParamStorageSnapshot
{
  uint32_t sequence;
  uint16_t count;
  uint16_t reserved;
  ParamStoredRecord records[ParamMaxCount];
};

static_assert(sizeof(ParamStoredRecord) == 24,
              "ParamStoredRecord format must stay stable");
