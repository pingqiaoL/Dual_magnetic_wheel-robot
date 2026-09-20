/** @file param_command_test.cpp
 * @brief 验证编号启动脚本使用的只读compare命令以及构型默认值语义。
 */
#include "robot/params/ParamSystem.hpp"
#include <assert.h>
#include <stdio.h>

/** 主机内存存储，替换实际Flash；命令解析与参数实现使用正式源码。 */
class CommandTestStorage final : public ParamStorage
{
public:
  /** 测试开始时没有持久化数据。 */
  bool load(ParamStorageSnapshot &) override { return false; }
  /** 不写入任何实际Flash。 */
  bool save(const ParamStorageSnapshot &) override { return true; }
};

/** 用内存后端装配主机参数系统。 */
bool paramSystemInitialize()
{
  static CommandTestStorage storage;
  return ParamManager::instance().initialize(storage);
}

/** 返回正式参数管理器。 */
ParamManager &params() { return ParamManager::instance(); }

/** 调用正式NSH入口。 */
extern "C" int param_main(int, char *[]);

/** 测试命令调用助手。 */
class ParamCommandTest
{
public:
  /** 比较成功返回0，错误或不相等返回非零；比较不能修改参数。 */
  static void run()
  {
    assert(paramSystemInitialize());
    const uint32_t generation = params().generation();
    assert(call("compare", "SYS_AUTOSTART", "1") == 0);
    assert(call("compare", "SYS_AUTOSTART", "0x1") == 0);
    assert(call("compare", "SYS_AUTOSTART", "2") != 0);
    assert(call("compare", "SYS_AUTOSTART", "1junk") != 0);
    assert(call("compare", "SYS_AUTOSTART", "2147483648") != 0);
    assert(call("compare", "NO_SUCH_PARAM", "1") != 0);
    assert(call("compare", "CA_YAW_R", "0") == 0);
    assert(call("compare", "CA_YAW_R", "nan") != 0);
    assert(params().generation() == generation);
    assert(call("set", "CA_YAW_R", "-0.5") == 0);
    assert(call("set-default", "CA_YAW_R", "0") == 0);
    assert(call("compare", "CA_YAW_R", "-0.5") == 0);
    puts("param compare and numbered-profile defaults test passed");
  }
private:
  /** 给现有C接口传递可写argv指针，不修改字符串本身。 */
  static int call(const char *action, const char *name, const char *value)
  {
    char *args[] = {const_cast<char *>("param"), const_cast<char *>(action),
                    const_cast<char *>(name), const_cast<char *>(value), nullptr};
    return param_main(4, args);
  }
};

/** 执行参数命令验收。 */
int main() { ParamCommandTest::run(); return 0; }
