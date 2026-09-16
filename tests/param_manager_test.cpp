/**
 * @file param_manager_test.cpp
 * @brief 使用内存后端验证参数类型、通知、保存、加载和自动保存。
 */

#include "robot/orb/Topics.hpp"
#include "robot/os/Clock.hpp"
#include "robot/params/ParamManager.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/** 用内存快照代替板载 Flash 的主机测试后端。 */
class MemoryParamStorage final : public ParamStorage
{
public:
  bool load(ParamStorageSnapshot &snapshot) override
  {
    if (!_valid)
      {
        return false;
      }
    snapshot = _snapshot;
    return true;
  }

  bool save(const ParamStorageSnapshot &snapshot) override
  {
    _snapshot = snapshot;
    _valid = true;
    ++saveCount;
    return true;
  }

  unsigned saveCount{0};

private:
  ParamStorageSnapshot _snapshot{};
  bool _valid{false};
};

/** 执行参数管理器的核心行为断言。 */
int main()
{
  MemoryParamStorage storage;
  ParamManager &manager = ParamManager::instance();
  assert(manager.initialize(storage));
  assert(manager.count() == 142);

  const ParamHandle minimum = manager.find("RC1_MIN");
  const ParamHandle rollMap = manager.find("RC_MAP_ROLL");
  const ParamHandle steeringCommandMax = manager.find("CAN_S0_PMAX");
  const ParamHandle steeringFeedbackMax = manager.find("CAN_S0_PFBMAX");
  assert(minimum != ParamInvalid);
  assert(rollMap != ParamInvalid);
  assert(steeringCommandMax != ParamInvalid);
  assert(steeringFeedbackMax != ParamInvalid);
  assert(manager.find("DOES_NOT_EXIST") == ParamInvalid);

  float floatValue = 0.0f;
  int32_t integerValue = 0;
  assert(manager.get(minimum, floatValue) && floatValue == 1000.0f);
  assert(manager.get(rollMap, integerValue) && integerValue == 1);
  assert(manager.get(steeringCommandMax, floatValue) && floatValue == 1.0f);
  assert(manager.get(steeringFeedbackMax, floatValue) &&
         floatValue == 12.5f);
  assert(!manager.get(minimum, integerValue));
  assert(!manager.set(minimum, static_cast<int32_t>(1045)));

  uorb::Subscription<ParameterUpdate> updates(parameterUpdateTopic());
  assert(manager.set(minimum, 1045.0f));
  ParameterUpdate update{};
  assert(updates.update(update));
  assert(!update.saved);
  assert(manager.dirty());

  assert(manager.set(rollMap, static_cast<int32_t>(2)));
  assert(manager.save());
  assert(storage.saveCount == 1);
  assert(!manager.dirty());
  assert(manager.storageSequence() == 1);

  assert(manager.set(minimum, 1100.0f));
  assert(manager.load());
  assert(manager.get(minimum, floatValue) && floatValue == 1045.0f);
  assert(manager.get(rollMap, integerValue) && integerValue == 2);

  assert(manager.set(minimum, 1050.0f));
  manager.pollAutoSave(os::Clock::nowMicroseconds() + 2000000ULL);
  assert(storage.saveCount == 2);
  assert(!manager.dirty());

  printf("[PASS] Parameter manager tests passed.\n");
  return 0;
}
