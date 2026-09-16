# DJI C-board BSP

This is the canonical out-of-tree NuttX BSP for the DJI RoboMaster Development Board Type C (`STM32F407IGH6`). It contains the 12 MHz-to-168 MHz clock setup, 1 MiB Flash/112 KiB main-SRAM linker map, USART6 NSH console, RGB status LED, user key, and verified USART/CAN/PWM pin selections.

The BSP is deliberately limited to board resources. DBUS decoding, MAVLink, CAN actuator protocols, PWM output policy, parameter storage and robot control belong to reusable drivers and modules outside this directory.

Use `configs/robot/defconfig` as the only editable board configuration. Run `tools/build.sh --clean` from the project root; generated NuttX `.config` files are build products.
