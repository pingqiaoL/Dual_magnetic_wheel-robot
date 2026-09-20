# 本文件将模块实现末尾的入口注册为七个NuttX NSH内置命令。
include $(APPDIR)/Make.defs

PROGNAME  = $(CONFIG_CBOARD_ROBOT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_SBUS_INPUT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_RC_UPDATE_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_PARAM_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_CONTROL_ALLOCATOR_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_CAN_OUTPUT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_COMMAND_PROGNAME)
PRIORITY  = $(CONFIG_CBOARD_ROBOT_PRIORITY) 100 100 100 100 100 100
STACKSIZE = $(CONFIG_CBOARD_ROBOT_STACKSIZE) 2048 2048 6144 2048 2048 2048
MODULE    = $(CONFIG_CBOARD_ROBOT)

CXXEXT = .cpp
CXXSRCS = Mutex.cpp Semaphore.cpp Clock.cpp Task.cpp
CXXSRCS += Topics.cpp sbus.cpp RcConfig.cpp
CXXSRCS += ParamManager.cpp FlashParamStorage.cpp ParamSystem.cpp
CXXSRCS += CommandRouter.cpp MixingOutput.cpp Allocation.cpp
CXXSRCS += ActuatorEffectivenessDualMagneticWheel.cpp
CXXSRCS += DamiaoProtocol.cpp
MAINSRC = RobotRuntime.cpp SbusInput.cpp RcUpdate.cpp param_main.cpp
MAINSRC += ControlAllocator.cpp CanOutput.cpp Command.cpp

CXXFLAGS += ${INCDIR_PREFIX}$(CURDIR)
# 将显式入口映射到Kconfig命令名称，保留原来的命令改名能力。
CXXFLAGS += -Drobot_main=$(subst ",,$(CONFIG_CBOARD_ROBOT_PROGNAME))_main
CXXFLAGS += -Dsbus_input_main=$(subst ",,$(CONFIG_CBOARD_SBUS_INPUT_PROGNAME))_main
CXXFLAGS += -Drc_update_main=$(subst ",,$(CONFIG_CBOARD_RC_UPDATE_PROGNAME))_main
CXXFLAGS += -Dparam_main=$(subst ",,$(CONFIG_CBOARD_PARAM_PROGNAME))_main
CXXFLAGS += -Dcontrol_allocator_main=$(subst ",,$(CONFIG_CBOARD_CONTROL_ALLOCATOR_PROGNAME))_main
CXXFLAGS += -Dcan_output_main=$(subst ",,$(CONFIG_CBOARD_CAN_OUTPUT_PROGNAME))_main
CXXFLAGS += -Dcommand_main=$(subst ",,$(CONFIG_CBOARD_COMMAND_PROGNAME))_main
VPATH += :robot/common:robot/os:robot/orb:robot/drivers/rc:robot/modules:robot/params
VPATH += :robot/configuration:robot/control:robot/output:protocol/can

include $(APPDIR)/Application.mk
