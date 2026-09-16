/****************************************************************************
 * platform/dji_cboard/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 ****************************************************************************/

#ifndef __PLATFORM_DJI_CBOARD_INCLUDE_BOARD_H
#define __PLATFORM_DJI_CBOARD_INCLUDE_BOARD_H

#include <nuttx/config.h>

/* Clocking: 12 MHz HSE -> 168 MHz SYSCLK. */

#define STM32_BOARD_XTAL              12000000ul
#define STM32_HSI_FREQUENCY           16000000ul
#define STM32_LSI_FREQUENCY           32000ul
#define STM32_HSE_FREQUENCY           STM32_BOARD_XTAL
#define STM32_LSE_FREQUENCY           32768ul

#define STM32_PLLCFG_PLLM             RCC_PLLCFG_PLLM(12)
#define STM32_PLLCFG_PLLN             RCC_PLLCFG_PLLN(336)
#define STM32_PLLCFG_PLLP             RCC_PLLCFG_PLLP_2
#define STM32_PLLCFG_PLLQ             RCC_PLLCFG_PLLQ(7)

#define STM32_SYSCLK_FREQUENCY        168000000ul
#define STM32_HCLK_FREQUENCY          STM32_SYSCLK_FREQUENCY
#define STM32_PCLK1_FREQUENCY         (STM32_HCLK_FREQUENCY / 4)
#define STM32_PCLK2_FREQUENCY         (STM32_HCLK_FREQUENCY / 2)

#define STM32_RCC_CFGR_HPRE           RCC_CFGR_HPRE_SYSCLK
#define STM32_RCC_CFGR_PPRE1          RCC_CFGR_PPRE1_HCLKd4
#define STM32_RCC_CFGR_PPRE2          RCC_CFGR_PPRE2_HCLKd2

#define STM32_APB1_TIM2_CLKIN         (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM3_CLKIN         (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM4_CLKIN         (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM5_CLKIN         (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM6_CLKIN         (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM7_CLKIN         (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM12_CLKIN        (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM13_CLKIN        (2 * STM32_PCLK1_FREQUENCY)
#define STM32_APB1_TIM14_CLKIN        (2 * STM32_PCLK1_FREQUENCY)

#define STM32_APB2_TIM1_CLKIN         (2 * STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM8_CLKIN         (2 * STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM9_CLKIN         (2 * STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM10_CLKIN        (2 * STM32_PCLK2_FREQUENCY)
#define STM32_APB2_TIM11_CLKIN        (2 * STM32_PCLK2_FREQUENCY)

#define BOARD_TIM1_FREQUENCY          STM32_APB2_TIM1_CLKIN
#define BOARD_TIM2_FREQUENCY          STM32_APB1_TIM2_CLKIN
#define BOARD_TIM3_FREQUENCY          STM32_APB1_TIM3_CLKIN
#define BOARD_TIM4_FREQUENCY          STM32_APB1_TIM4_CLKIN
#define BOARD_TIM5_FREQUENCY          STM32_APB1_TIM5_CLKIN
#define BOARD_TIM6_FREQUENCY          STM32_APB1_TIM6_CLKIN
#define BOARD_TIM7_FREQUENCY          STM32_APB1_TIM7_CLKIN
#define BOARD_TIM8_FREQUENCY          STM32_APB2_TIM8_CLKIN
#define BOARD_TIM9_FREQUENCY          STM32_APB2_TIM9_CLKIN
#define BOARD_TIM10_FREQUENCY         STM32_APB2_TIM10_CLKIN
#define BOARD_TIM11_FREQUENCY         STM32_APB2_TIM11_CLKIN
#define BOARD_TIM12_FREQUENCY         STM32_APB1_TIM12_CLKIN
#define BOARD_TIM13_FREQUENCY         STM32_APB1_TIM13_CLKIN
#define BOARD_TIM14_FREQUENCY         STM32_APB1_TIM14_CLKIN

/* Serial resources.  USART3 receives the board-inverted DBUS/SBUS signal;
 * USART1 is reserved for MAVLink and USART6 is the NSH console.
 */

#define GPIO_USART1_RX                (GPIO_USART1_RX_2 | GPIO_SPEED_100MHz) /* PB7  */
#define GPIO_USART1_TX                (GPIO_USART1_TX_1 | GPIO_SPEED_100MHz) /* PA9  */
#define GPIO_USART3_RX                (GPIO_USART3_RX_2 | GPIO_SPEED_100MHz) /* PC11 */
#define GPIO_USART3_TX                (GPIO_USART3_TX_2 | GPIO_SPEED_100MHz) /* PC10 */
#define GPIO_USART6_RX                (GPIO_USART6_RX_2 | GPIO_SPEED_100MHz) /* PG9  */
#define GPIO_USART6_TX                (GPIO_USART6_TX_2 | GPIO_SPEED_100MHz) /* PG14 */

/* Parameter storage uses the final two 128 KiB sectors of the 1 MiB Flash.
 * The linker script ends firmware at 0x080c0000, so these slots cannot overlap
 * executable code.  Two slots provide power-loss-safe alternating saves.
 */

#define BOARD_PARAM_FLASH_SLOT_A      0x080c0000ul /* Sector 10 */
#define BOARD_PARAM_FLASH_SLOT_B      0x080e0000ul /* Sector 11 */
#define BOARD_PARAM_FLASH_SLOT_SIZE   (128ul * 1024ul)

/* CAN resources. */

#define GPIO_CAN1_RX                  (GPIO_CAN1_RX_3 | GPIO_SPEED_50MHz) /* PD0 */
#define GPIO_CAN1_TX                  (GPIO_CAN1_TX_3 | GPIO_SPEED_50MHz) /* PD1 */
#define GPIO_CAN2_RX                  (GPIO_CAN2_RX_2 | GPIO_SPEED_50MHz) /* PB5 */
#define GPIO_CAN2_TX                  (GPIO_CAN2_TX_2 | GPIO_SPEED_50MHz) /* PB6 */

/* PWM-capable actuator outputs. */

#define GPIO_TIM1_CH1OUT              (GPIO_TIM1_CH1OUT_2 | GPIO_SPEED_50MHz) /* PE9  */
#define GPIO_TIM1_CH2OUT              (GPIO_TIM1_CH2OUT_2 | GPIO_SPEED_50MHz) /* PE11 */
#define GPIO_TIM1_CH3OUT              (GPIO_TIM1_CH3OUT_2 | GPIO_SPEED_50MHz) /* PE13 */
#define GPIO_TIM1_CH4OUT              (GPIO_TIM1_CH4OUT_2 | GPIO_SPEED_50MHz) /* PE14 */
#define GPIO_TIM8_CH1OUT              (GPIO_TIM8_CH1OUT_1 | GPIO_SPEED_50MHz) /* PC6  */
#define GPIO_TIM8_CH2OUT              (GPIO_TIM8_CH2OUT_2 | GPIO_SPEED_50MHz) /* PI6  */
#define GPIO_TIM8_CH3OUT              (GPIO_TIM8_CH3OUT_2 | GPIO_SPEED_50MHz) /* PI7  */

/* RGB LED indices and masks.  The MCU drives an NMOS gate, so GPIO high
 * turns the corresponding LED channel on.
 */

#define BOARD_LED_BLUE                0
#define BOARD_LED_GREEN               1
#define BOARD_LED_RED                 2
#define BOARD_NLEDS                   3

#define BOARD_LED_BLUE_BIT            (1 << BOARD_LED_BLUE)
#define BOARD_LED_GREEN_BIT           (1 << BOARD_LED_GREEN)
#define BOARD_LED_RED_BIT             (1 << BOARD_LED_RED)

/* NuttX automatic LED event mapping. */

#define LED_STARTED                   0
#define LED_HEAPALLOCATE              1
#define LED_IRQSENABLED               2
#define LED_STACKCREATED              3
#define LED_INIRQ                     4
#define LED_SIGNAL                    5
#define LED_ASSERTION                 6
#define LED_PANIC                     7

/* One active-low user button: KEY on PA0. */

#define BUTTON_USER                   0
#define NUM_BUTTONS                   1
#define BUTTON_USER_BIT               (1 << BUTTON_USER)

#endif /* __PLATFORM_DJI_CBOARD_INCLUDE_BOARD_H */
