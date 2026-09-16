# C-board hardware resource map

This map separates confirmed board wiring from planned software ownership. Connector orientation must be checked against the physical connector and continuity before making a harness.

| Function | MCU resource | Pin(s) | Planned owner | Status |
|---|---|---|---|---|
| External oscillator | HSE | PH0/PH1 | C-board BSP clock setup | Confirmed, 12 MHz |
| NSH console | USART6 | TX PG14, RX PG9 | NuttX serial/console | Planned |
| MK32 SBUS input | USART3 RX | PC11 | `sbus_input` | Driver and parser complete; board acceptance pending |
| Jetson MAVLink | USART1 | TX PA9, RX PB7 | `mavlink` | Reserved for version 2 |
| DM motor bus | CAN1 | TX PD1, RX PD0 | `can_output -b 1` | Planned primary bus |
| Optional actuator bus | CAN2 | TX PB6, RX PB5 | `can_output -b 2` | Reserved |
| PWM outputs 1-4 | TIM1 CH1-4 | PE9, PE11, PE13, PE14 | `pwm_output` | Confirmed board route |
| PWM outputs 5-7 | TIM8 CH1-3 | PC6, PI6, PI7 | `pwm_output` | Confirmed board route |
| RGB LED | GPIO | PH10, PH11, PH12 | BSP/user LED | Confirmed |
| User key | GPIO input | PA0 | BSP/button test | Confirmed, active low |
| Battery voltage | ADC | PF10 | `power_monitor` | Confirmed route; scale pending |
| BMI088 | SPI1 | board-specific chip selects/IRQs | future IMU driver | Deferred |

## Interface constraints

- The C-board hardware inverts the signal on the connector labeled DBUS before USART3 RX. The MK32 standard inverted SBUS signal therefore reaches USART3 at normal UART polarity.
- SBUS is configured as 100000 baud, eight data bits, even parity, two stop bits.
- CAN1 is a two-pin signal connector: pin 1 CANL and pin 2 CANH. Motor power is supplied separately.
- TIM1 channels share one PWM period; TIM8 channels share another PWM period.
- The enclosure UART labels do not equal the STM32 peripheral numbers. Software names use STM32 peripheral numbers.
- The parameter Flash sectors remain unassigned until the Step 2 linker map proves the firmware does not overlap them.
