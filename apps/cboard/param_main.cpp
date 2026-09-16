/**
 * @file param_main.cpp
 * @brief 提供 PX4 风格的 NSH 参数查看、修改、重置、加载和保存命令。
 */

#include "robot/params/ParamSystem.hpp"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
/** 打印命令帮助。 */
int printUsage(const char *reason = nullptr)
{
  if (reason != nullptr)
    {
      fprintf(stderr, "%s\n", reason);
    }

  printf("usage: param {show [prefix*]|get <name>|set <name> <value>|"
         "set-default <name> <value>|reset <name|all>|save|load|status}\n");
  return reason == nullptr ? 0 : -1;
}

/** 支持完整名称、前缀和末尾星号前缀查询。 */
bool nameMatches(const char *name, const char *pattern)
{
  if (pattern == nullptr || pattern[0] == '\0' || strcmp(pattern, "*") == 0)
    {
      return true;
    }

  const size_t length = strlen(pattern);
  if (pattern[length - 1] == '*')
    {
      return strncmp(name, pattern, length - 1) == 0;
    }
  return strcmp(name, pattern) == 0;
}

/** 按参数定义的类型打印当前值。 */
bool printParameter(ParamManager &manager, ParamHandle handle)
{
  const ParamDefinition *definition = manager.definition(handle);
  if (definition == nullptr)
    {
      return false;
    }

  if (definition->type == ParamType::Int32)
    {
      int32_t value = 0;
      if (!manager.get(handle, value))
        {
          return false;
        }
      printf("%-16s %ld\n", definition->name, static_cast<long>(value));
    }
  else
    {
      float value = 0.0f;
      if (!manager.get(handle, value))
        {
          return false;
        }
      printf("%-16s %.6g\n", definition->name,
             static_cast<double>(value));
    }
  return true;
}

/** 严格解析并设置与参数类型匹配的文本数值。 */
bool setFromText(ParamManager &manager, ParamHandle handle, const char *text)
{
  const ParamDefinition *definition = manager.definition(handle);
  if (definition == nullptr || text == nullptr)
    {
      return false;
    }

  errno = 0;
  char *end = nullptr;
  if (definition->type == ParamType::Int32)
    {
      const long parsed = strtol(text, &end, 0);
      if (errno != 0 || end == text || *end != '\0' ||
          parsed < INT32_MIN || parsed > INT32_MAX)
        {
          return false;
        }
      return manager.set(handle, static_cast<int32_t>(parsed));
    }

  const float parsed = strtof(text, &end);
  if (errno != 0 || end == text || *end != '\0')
    {
      return false;
    }
  return manager.set(handle, parsed);
}

/** 严格解析文本，并把它设置为构型默认值。 */
bool setDefaultFromText(ParamManager &manager, ParamHandle handle,
                        const char *text)
{
  const ParamDefinition *definition = manager.definition(handle);
  if (definition == nullptr || text == nullptr)
    {
      return false;
    }

  errno = 0;
  char *end = nullptr;
  if (definition->type == ParamType::Int32)
    {
      const long parsed = strtol(text, &end, 0);
      return errno == 0 && end != text && *end == '\0' &&
             parsed >= INT32_MIN && parsed <= INT32_MAX &&
             manager.setDefault(handle, static_cast<int32_t>(parsed));
    }

  const float parsed = strtof(text, &end);
  return errno == 0 && end != text && *end == '\0' &&
         manager.setDefault(handle, parsed);
}
} // namespace

/** 解析 param 子命令并操作共享参数管理器。 */
extern "C" int main(int argc, char *argv[])
{
  if (!paramSystemInitialize())
    {
      fprintf(stderr, "parameter system initialization failed\n");
      return -1;
    }

  if (argc < 2 || strcmp(argv[1], "help") == 0 ||
      strcmp(argv[1], "-h") == 0)
    {
      return printUsage();
    }

  ParamManager &manager = params();
  if (strcmp(argv[1], "show") == 0)
    {
      const char *pattern = argc >= 3 ? argv[2] : "*";
      unsigned shown = 0;
      for (size_t index = 0; index < manager.count(); ++index)
        {
          const ParamHandle handle = static_cast<ParamHandle>(index);
          const ParamDefinition *definition = manager.definition(handle);
          if (definition != nullptr && nameMatches(definition->name, pattern) &&
              printParameter(manager, handle))
            {
              ++shown;
            }
        }
      printf("%u parameters shown\n", shown);
      return 0;
    }

  if (strcmp(argv[1], "get") == 0 && argc >= 3)
    {
      const ParamHandle handle = manager.find(argv[2]);
      return printParameter(manager, handle) ? 0 : printUsage("parameter not found");
    }

  if (strcmp(argv[1], "set") == 0 && argc >= 4)
    {
      const ParamHandle handle = manager.find(argv[2]);
      if (!setFromText(manager, handle, argv[3]))
        {
          return printUsage("parameter value or type is invalid");
        }
      (void)printParameter(manager, handle);
      printf("autosave pending\n");
      return 0;
    }

  if (strcmp(argv[1], "set-default") == 0 && argc >= 4)
    {
      const ParamHandle handle = manager.find(argv[2]);
      if (!setDefaultFromText(manager, handle, argv[3]))
        {
          return printUsage("parameter default value or type is invalid");
        }
      (void)printParameter(manager, handle);
      return 0;
    }

  if (strcmp(argv[1], "reset") == 0 && argc >= 3)
    {
      if (strcmp(argv[2], "all") == 0)
        {
          manager.resetAll();
          printf("all parameters reset; autosave pending\n");
          return 0;
        }

      const ParamHandle handle = manager.find(argv[2]);
      if (!manager.reset(handle))
        {
          return printUsage("parameter not found");
        }
      (void)printParameter(manager, handle);
      printf("autosave pending\n");
      return 0;
    }

  if (strcmp(argv[1], "save") == 0)
    {
      const bool success = manager.save();
      printf(success ? "parameters saved\n" : "parameter save failed\n");
      return success ? 0 : -1;
    }

  if (strcmp(argv[1], "load") == 0)
    {
      const bool success = manager.load();
      printf(success ? "parameters loaded\n" : "no valid parameter snapshot\n");
      return success ? 0 : -1;
    }

  if (strcmp(argv[1], "status") == 0)
    {
      printf("parameters: %lu\n", static_cast<unsigned long>(manager.count()));
      printf("generation: %lu\n",
             static_cast<unsigned long>(manager.generation()));
      printf("storage sequence: %lu\n",
             static_cast<unsigned long>(manager.storageSequence()));
      printf("dirty: %s\n", manager.dirty() ? "yes" : "no");
      return 0;
    }

  return printUsage("unknown param command");
}
