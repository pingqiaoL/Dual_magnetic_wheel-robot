/** @file module_params_test.cpp
 * @brief 验证父节点递归更新子节点的参数缓存。
 */
#include "robot/params/ModuleParams.hpp"
#include "robot/params/Param.hpp"
#include <assert.h>
#include <stdio.h>

class ChildParams final : public ModuleParams
{
public:
  explicit ChildParams(ModuleParams *parent)
      : ModuleParams(parent), armSwitch(this, "CA_ARM_SW", 1),
        timeout(this, "FS_RC_TMO", 0.5f) {}
  ParamInt armSwitch;
  ParamFloat timeout;
};

int main()
{
  ModuleParams root;
  ChildParams child(&root);
  ParamManager &manager = ParamManager::instance();
  assert(manager.set(manager.find("CA_ARM_SW"), int32_t{3}));
  assert(manager.set(manager.find("FS_RC_TMO"), 1.25f));
  assert(root.updateParams());
  assert(child.armSwitch.get() == 3);
  assert(child.timeout.get() == 1.25f);
  puts("ModuleParams recursive update test passed");
  return 0;
}
