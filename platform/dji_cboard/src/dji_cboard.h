/****************************************************************************
 * platform/dji_cboard/src/dji_cboard.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __PLATFORM_DJI_CBOARD_SRC_DJI_CBOARD_H
#define __PLATFORM_DJI_CBOARD_SRC_DJI_CBOARD_H

#include <nuttx/config.h>

#include "stm32_gpio.h"

#define GPIO_LED_BLUE   (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHz | \
                         GPIO_OUTPUT_CLEAR | GPIO_PORTH | GPIO_PIN10)
#define GPIO_LED_GREEN  (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHz | \
                         GPIO_OUTPUT_CLEAR | GPIO_PORTH | GPIO_PIN11)
#define GPIO_LED_RED    (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_2MHz | \
                         GPIO_OUTPUT_CLEAR | GPIO_PORTH | GPIO_PIN12)

#define GPIO_BTN_USER   (GPIO_INPUT | GPIO_PULLUP | GPIO_EXTI | \
                         GPIO_PORTA | GPIO_PIN0)

#ifdef CONFIG_ARCH_LEDS
void stm32_led_initialize(void);
#endif

int stm32_bringup(void);

#ifdef CONFIG_CAN
int stm32_can_setup(void);
#endif

#endif /* __PLATFORM_DJI_CBOARD_SRC_DJI_CBOARD_H */
