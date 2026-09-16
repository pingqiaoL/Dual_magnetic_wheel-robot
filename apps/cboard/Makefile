# 本文件把基础任务和遥控链路编译为三个 NuttX NSH 内置命令。
include $(APPDIR)/Make.defs

PROGNAME  = $(CONFIG_CBOARD_ROBOT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_SBUS_INPUT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_RC_UPDATE_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_PARAM_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_CONTROL_ALLOCATOR_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_CAN_OUTPUT_PROGNAME)
PRIORITY  = $(CONFIG_CBOARD_ROBOT_PRIORITY) 100 100 100 100 100
STACKSIZE = $(CONFIG_CBOARD_ROBOT_STACKSIZE) 2048 2048 6144 2048 2048
MODULE    = $(CONFIG_CBOARD_ROBOT)

CXXEXT = .cpp
CXXSRCS = Mutex.cpp Semaphore.cpp Clock.cpp Task.cpp
CXXSRCS += Topics.cpp sbus.cpp RcConfig.cpp
CXXSRCS += ParamManager.cpp FlashParamStorage.cpp ParamSystem.cpp
CXXSRCS += RobotRuntime.cpp SbusInput.cpp RcUpdate.cpp
CXXSRCS += ClimbotConfiguration.cpp ClimbotAllocation.cpp ControlAllocator.cpp
CXXSRCS += DamiaoProtocol.cpp CanOutput.cpp
MAINSRC = robot_main.cpp sbus_input_main.cpp rc_update_main.cpp param_main.cpp
MAINSRC += control_allocator_main.cpp can_output_main.cpp

CXXFLAGS += ${INCDIR_PREFIX}$(CURDIR)
VPATH += :robot/common:robot/os:robot/orb:robot/drivers/rc:robot/modules:robot/params
VPATH += :robot/configuration:robot/control:robot/output:protocol/can

include $(APPDIR)/Application.mk
