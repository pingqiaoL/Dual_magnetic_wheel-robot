# Development baseline

## Fixed inputs

| Item | Baseline |
|---|---|
| Target board | DJI RoboMaster Development Board Type C |
| MCU | STM32F407IGH6 |
| HSE | 12 MHz |
| RTOS | Apache NuttX 13.0.0 |
| NuttX apps | Apache NuttX Apps 13.0.0 |
| Firmware language | C++ for robot modules; C is allowed at NuttX/BSP boundaries |
| Reference toolchain | Arm GNU Toolchain 14.3.rel1 (`arm-none-eabi`) |
| Reference host | Ubuntu 24.04 LTS/WSL2, or Windows with MSYS2 |
| Shell startup | NuttX NSH with ROMFS `rcS` |
| Internal messaging | Project uORB-style typed publish/subscribe layer |
| Scheduling | Ordinary NuttX threads; no PX4 WorkQueue dependency |

The exact tags and commits are machine-readable in `config/versions.env`.

## Build contract

`tools/build.sh` is the canonical build entry point. `tools/build.ps1` delegates to it through WSL when available, or through MSYS2 and the STM32CubeIDE Arm toolchain on Windows. A build is successful only when it produces the following under `artifacts/`:

- `nuttx` ELF image;
- `nuttx.bin` binary image;
- `nuttx.hex` Intel HEX image when supported by the configuration;
- `nuttx.map` linker map;
- `size.txt` section-size report;
- `build-info.txt` containing upstream commits and tool versions.

The custom board configuration lives in `platform/dji_cboard`. The build script copies its canonical `robot` defconfig into the verified upstream source tree, runs `olddefconfig`, builds NuttX, and collects the outputs.

## Version update rule

NuttX and NuttX Apps must move together to the same release. Updating a release requires updating the Apache archive URL and SHA-512, annotated tag-object ID, peeled commit ID, rerunning the layout tests, building the firmware, and recording hardware regression results. Development must not track an unpinned `master` branch.

## Hardware assumptions still open

- number of DM motors;
- 24 V or 48 V DM-J4310-2EC variant;
- motor CAN IDs and selected control mode;
- electric pushrod controller and feedback type;
- PWM or serial servo models;
- exact battery and power-distribution design;
- final internal Flash sectors for parameter A/B storage.

Those values belong in robot parameters and board configuration after the hardware is selected. They must not be guessed in the BSP.
