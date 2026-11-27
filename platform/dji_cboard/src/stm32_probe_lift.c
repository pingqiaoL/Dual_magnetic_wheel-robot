#include "dji_cboard.h"
#include <arch/board/probe_lift.h>

int board_probe_lift_initialize(void)
{
  int ret = stm32_configgpio(GPIO_LIFT_IN1);
  if (ret < 0) { return ret; }
  ret = stm32_configgpio(GPIO_LIFT_IN2);
  if (ret < 0) { return ret; }
  return board_probe_lift_initialize_switch();
}

int board_probe_lift_initialize_switch(void)
{
  return stm32_configgpio(GPIO_LIFT_WORK);
}

void board_probe_lift_write(bool in1, bool in2)
{
  stm32_gpiowrite(GPIO_LIFT_IN1, in1);
  stm32_gpiowrite(GPIO_LIFT_IN2, in2);
}

bool board_probe_lift_work_pressed(void)
{
  return !stm32_gpioread(GPIO_LIFT_WORK);
}
