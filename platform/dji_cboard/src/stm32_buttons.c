/****************************************************************************
 * platform/dji_cboard/src/stm32_buttons.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/board.h>
#include <arch/board/board.h>

#include <errno.h>
#include <stdint.h>

#include "stm32_gpio.h"
#include "dji_cboard.h"

uint32_t board_button_initialize(void)
{
  stm32_configgpio(GPIO_BTN_USER);
  return NUM_BUTTONS;
}

uint32_t board_buttons(void)
{
  return stm32_gpioread(GPIO_BTN_USER) ? 0 : BUTTON_USER_BIT;
}

#ifdef CONFIG_ARCH_IRQBUTTONS
int board_button_irq(int id, xcpt_t irqhandler, FAR void *arg)
{
  if (id != BUTTON_USER)
    {
      return -EINVAL;
    }

  return stm32_gpiosetevent(GPIO_BTN_USER, true, true, true,
                            irqhandler, arg);
}
#endif
