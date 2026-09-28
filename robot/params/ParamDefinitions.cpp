/**
 * @file ParamDefinitions.cpp
 * @brief 使用统一宏集中注册项目参数名称、类型和固件默认值。
 */

#include "robot/params/ParamDefinitions.hpp"
#include "robot/params/ParamManager.hpp"

#include <stdio.h>

#define PARAM_DEFINE_INT32(name, value) manager.addInt(#name, value)
#define PARAM_DEFINE_FLOAT(name, value) manager.addFloat(#name, value)

void buildParamDefinitions(ParamManager &manager)
{
  char name[ParamNameLength];
  for (unsigned channel = 1; channel <= 16; ++channel)
    {
      snprintf(name, sizeof(name), "RC%u_MIN", channel);
      manager.addFloat(name, 1000.0f);
      snprintf(name, sizeof(name), "RC%u_MAX", channel);
      manager.addFloat(name, 2000.0f);
      snprintf(name, sizeof(name), "RC%u_TRIM", channel);
      manager.addFloat(name, 1500.0f);
      snprintf(name, sizeof(name), "RC%u_DZ", channel);
      manager.addFloat(name, 20.0f);
      snprintf(name, sizeof(name), "RC%u_REV", channel);
      manager.addFloat(name, 1.0f);
    }

  PARAM_DEFINE_INT32(RC_MAP_THROTTLE, 3);
  PARAM_DEFINE_INT32(RC_MAP_ROLL, 1);
  PARAM_DEFINE_INT32(RC_MAP_PITCH, 2);
  PARAM_DEFINE_INT32(RC_MAP_YAW, 4);
  for (unsigned switchIndex = 1; switchIndex <= 12; ++switchIndex)
    {
      snprintf(name, sizeof(name), "RC_MAP_SW%u", switchIndex);
      manager.addInt(name, static_cast<int32_t>(switchIndex + 4U));
    }
  PARAM_DEFINE_INT32(RC_CHAN_CNT, 16);

  PARAM_DEFINE_INT32(SYS_AUTOSTART, 1);
  PARAM_DEFINE_INT32(CA_AIRFRAME, 1);
  PARAM_DEFINE_FLOAT(CA_THR_F, 1.0f);
  PARAM_DEFINE_FLOAT(CA_THR_R, 1.0f);
  PARAM_DEFINE_FLOAT(CA_YAW_F, 1.0f);
  PARAM_DEFINE_FLOAT(CA_YAW_R, 0.0f);
  PARAM_DEFINE_INT32(CA_ARM_SW, 1);

  PARAM_DEFINE_FLOAT(FS_RC_TMO, 0.5f);
  PARAM_DEFINE_INT32(FS_MOT_ACT, 1);
  PARAM_DEFINE_FLOAT(FS_MOT_VAL, 0.0f);
  PARAM_DEFINE_INT32(FS_STR_ACT, 0);
  PARAM_DEFINE_FLOAT(FS_STR_VAL, 0.0f);
  PARAM_DEFINE_INT32(OUT_FAIL_ACT, 2);

  for (unsigned index = 0; index < 2; ++index)
    {
      snprintf(name, sizeof(name), "CAN_M%u_FUNC", index);
      manager.addInt(name, static_cast<int32_t>(101U + index));
      snprintf(name, sizeof(name), "CAN_M%u_PROTO", index);
      manager.addInt(name, 1);
      snprintf(name, sizeof(name), "CAN_M%u_TYPE", index);
      manager.addInt(name, 2);
      snprintf(name, sizeof(name), "CAN_M%u_ID", index);
      manager.addInt(name, static_cast<int32_t>(index + 1U));
      snprintf(name, sizeof(name), "CAN_M%u_FBID", index);
      manager.addInt(name, 0);
      snprintf(name, sizeof(name), "CAN_M%u_MODE", index);
      manager.addInt(name, 3);
      snprintf(name, sizeof(name), "CAN_M%u_REV", index);
      manager.addFloat(name, 1.0f);
      snprintf(name, sizeof(name), "CAN_M%u_PMAX", index);
      manager.addFloat(name, 12.5f);
      snprintf(name, sizeof(name), "CAN_M%u_VMAX", index);
      manager.addFloat(name, 30.0f);
      snprintf(name, sizeof(name), "CAN_M%u_TMAX", index);
      manager.addFloat(name, 10.0f);
    }

  for (unsigned index = 0; index < 2; ++index)
    {
      snprintf(name, sizeof(name), "CAN_S%u_FUNC", index);
      manager.addInt(name, static_cast<int32_t>(201U + index));
      snprintf(name, sizeof(name), "CAN_S%u_PROTO", index);
      manager.addInt(name, 1);
      snprintf(name, sizeof(name), "CAN_S%u_TYPE", index);
      manager.addInt(name, 1);
      snprintf(name, sizeof(name), "CAN_S%u_ID", index);
      manager.addInt(name, static_cast<int32_t>(index + 3U));
      snprintf(name, sizeof(name), "CAN_S%u_FBID", index);
      manager.addInt(name, 0);
      snprintf(name, sizeof(name), "CAN_S%u_MODE", index);
      manager.addInt(name, 2);
      snprintf(name, sizeof(name), "CAN_S%u_REV", index);
      manager.addFloat(name, 1.0f);
      snprintf(name, sizeof(name), "CAN_S%u_PMIN", index);
      manager.addFloat(name, -1.0f);
      snprintf(name, sizeof(name), "CAN_S%u_PMAX", index);
      manager.addFloat(name, 1.0f);
      snprintf(name, sizeof(name), "CAN_S%u_PFBMAX", index);
      manager.addFloat(name, 12.5f);
      snprintf(name, sizeof(name), "CAN_S%u_VMAX", index);
      manager.addFloat(name, 5.0f);
      snprintf(name, sizeof(name), "CAN_S%u_TMAX", index);
      manager.addFloat(name, 10.0f);
    }
}

#undef PARAM_DEFINE_INT32
#undef PARAM_DEFINE_FLOAT
