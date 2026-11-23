#pragma once
#include <stddef.h>
namespace robot_wq
{
struct wq_config_t
{
  const char *name;
  int priority; // Existing NuttX absolute priority, deliberately preserved.
  size_t stacksize;
};
namespace wq_configurations
{
extern const wq_config_t safety;
extern const wq_config_t control;
}
}
