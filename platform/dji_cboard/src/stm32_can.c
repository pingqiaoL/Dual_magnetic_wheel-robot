/****************************************************************************
 * platform/dji_cboard/src/stm32_can.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Register bxCAN1 on PD0/PD1 as the NuttX character device /dev/can0.
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_CAN

#include <errno.h>
#include <nuttx/can/can.h>

#include "stm32_can.h"

int stm32_can_setup(void)
{
#ifdef CONFIG_STM32_CAN1
  struct can_dev_s *can = stm32_caninitialize(1);
  if (can == NULL)
    {
      return -ENODEV;
    }

  return can_register("/dev/can0", can);
#else
  return -ENODEV;
#endif
}

#endif /* CONFIG_CAN */
