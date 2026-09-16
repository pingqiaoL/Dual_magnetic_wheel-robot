/****************************************************************************
 * platform/dji_cboard/src/stm32_boot.c
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#include <nuttx/config.h>

#include "dji_cboard.h"

void stm32_boardinitialize(void)
{
#ifdef CONFIG_ARCH_LEDS
  stm32_led_initialize();
#endif
}

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void)
{
  (void)stm32_bringup();
}
#endif
