/****************************************************************************
 * platform/dji_cboard/src/stm32_autoleds.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/board.h>
#include <arch/board/board.h>

#include <stdbool.h>

#include "stm32_gpio.h"
#include "dji_cboard.h"

static void dji_cboard_set_rgb(bool red, bool green, bool blue)
{
  stm32_gpiowrite(GPIO_LED_RED, red);
  stm32_gpiowrite(GPIO_LED_GREEN, green);
  stm32_gpiowrite(GPIO_LED_BLUE, blue);
}

void stm32_led_initialize(void)
{
  stm32_configgpio(GPIO_LED_BLUE);
  stm32_configgpio(GPIO_LED_GREEN);
  stm32_configgpio(GPIO_LED_RED);
  dji_cboard_set_rgb(false, false, false);
}

void board_autoled_on(int led)
{
  switch (led)
    {
      case LED_STARTED:
        dji_cboard_set_rgb(false, false, true);
        break;

      case LED_HEAPALLOCATE:
      case LED_STACKCREATED:
        dji_cboard_set_rgb(false, true, false);
        break;

      case LED_INIRQ:
      case LED_PANIC:
        stm32_gpiowrite(GPIO_LED_RED, true);
        break;

      default:
        break;
    }
}

void board_autoled_off(int led)
{
  switch (led)
    {
      case LED_STARTED:
        stm32_gpiowrite(GPIO_LED_BLUE, false);
        break;

      case LED_HEAPALLOCATE:
      case LED_STACKCREATED:
        stm32_gpiowrite(GPIO_LED_GREEN, false);
        break;

      case LED_INIRQ:
      case LED_PANIC:
        stm32_gpiowrite(GPIO_LED_RED, false);
        break;

      default:
        break;
    }
}
