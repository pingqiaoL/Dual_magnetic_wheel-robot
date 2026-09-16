# CBoard thickness-robot firmware

This repository is the lower-controller firmware workspace for the coating-thickness crawler robot.

The first hardware target is the DJI RoboMaster Development Board Type C (`STM32F407IGH6`, 12 MHz HSE). The operating system is Apache NuttX. Robot modules are written in C++ and started as NuttX NSH built-in applications.

## Current milestone

Step 2 adds the custom `dji_cboard` NuttX BSP. It configures the STM32F407IGH6 clock and memory map, USART6 NSH console, board LEDs and user key, and reserves the verified USART/CAN/PWM pin mappings used by later drivers.

Step 3 adds a header-only CRTP `ModuleBase<T>` modeled after the supplied PX4 module pattern, NuttX OS wrappers, the `robot start|stop|status` built-in command, and a ROMFS `rcS` that starts the hardware-independent runtime automatically.

Step 4 adds the MK32 SBUS receive chain through `rc_update`: USART3 SBUS decoding publishes `input_rc`; a lightweight uORB bus decouples the tasks; calibration, reversal, deadzone and channel mapping produce normalized `rc_channels`, `manual_control` and `manual_control_switches` topics.

Step 6 adds the Climbot actuator chain: `control_allocator` publishes
`actuator_motors` and `actuator_servos`; `can_output` selects the configured
CAN motor protocol and uses the NuttX `/dev/can0` character driver. The first
protocol implementation supports Damiao velocity and position-velocity modes,
management frames, and feedback decoding. Output requires both the RC arm
switch and the explicit `can_output enable` NSH command.

## Quick start

On Windows PowerShell:

```powershell
.\tools\setup_build_env.ps1
.\tools\check_env.ps1
.\tools\bootstrap.ps1
.\tests\step1_layout.ps1
.\tests\step2_bsp.ps1
.\tests\step3_runtime.ps1
.\tests\step4_rc_input.ps1
.\tests\step5_params.ps1
.\tests\step6_can_motor.ps1
.\tools\build.ps1 -Clean -Jobs 4
```

The PowerShell build uses WSL2 when a complete WSL toolchain is available. Otherwise it uses MSYS2 at `C:\msys64` together with the Arm GCC bundled in STM32CubeIDE. Firmware files are written to `artifacts/`.

On a native Linux machine:

```bash
./tools/check_env.sh
./tools/bootstrap.sh
./tools/build.sh
```

The build command configures `dji_cboard:robot`, builds NuttX, and copies user-facing binaries and memory reports into `artifacts/`.

## Source ownership

- `upstream/` contains downloaded Apache NuttX sources and is not edited as project source.
- `platform/dji_cboard/` is the canonical custom-board BSP source.
- `robot/` contains reusable OS-independent C++ robot code.
- `apps/` contains NSH module entry points and startup integration.
- `startup/` contains the planned `rcS` startup scripts.
- `config/` records project-wide resource and version decisions.

See `docs/development-baseline.md`, `docs/hardware-resource-map.md`,
`docs/source-layout.md`, `docs/step2-bsp.md`, and `docs/step4-rc-input.md`
before adding drivers or upper-level control modules.
