# 本文件将模块实现末尾的入口注册为 NuttX NSH 内置命令。
include $(APPDIR)/Make.defs

PROGNAME  = $(CONFIG_CBOARD_ROBOT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_SBUS_INPUT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_RC_UPDATE_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_PARAM_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_CONTROL_ALLOCATOR_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_CAN_OUTPUT_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_COMMAND_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_FAILSAFE_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_ROBOT_CONTROL_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_THICKNESS_CONTROL_PROGNAME)
PROGNAME += $(CONFIG_CBOARD_LINK_PROGNAME)
PRIORITY  = $(CONFIG_CBOARD_ROBOT_PRIORITY) 100 100 100 100 100 100 100 100 100 100
STACKSIZE = $(CONFIG_CBOARD_ROBOT_STACKSIZE) 2048 2048 6144 2048 2048 2048 2048 2048 2048 2048
MODULE    = $(CONFIG_CBOARD_ROBOT)

CXXEXT = .cpp
CXXSRCS = Mutex.cpp Semaphore.cpp Clock.cpp Task.cpp
CXXSRCS += Topics.cpp sbus.cpp RcConfig.cpp
CXXSRCS += ParamManager.cpp ParamDefinitions.cpp ModuleParams.cpp
CXXSRCS += FlashParamStorage.cpp ParamSystem.cpp
CXXSRCS += MixingOutput.cpp Allocation.cpp
CXXSRCS += ActuatorEffectivenessDualMagneticWheel.cpp
CXXSRCS += DamiaoProtocol.cpp
CXXSRCS += ProbeLift.cpp ProbeLiftBench.cpp
CXXSRCS += WorkItem.cpp ScheduledWorkItem.cpp WorkQueue.cpp
CXXSRCS += WorkQueueManager.cpp WorkQueueConfigurations.cpp
MAINSRC = RobotRuntime.cpp SbusInput.cpp RcUpdate.cpp param_main.cpp
MAINSRC += ControlAllocator.cpp CanOutput.cpp Command.cpp
MAINSRC += Failsafe.cpp
MAINSRC += RobotControl.cpp
MAINSRC += ThicknessControl.cpp CBoardLink.cpp

CXXFLAGS += -DCBOARD_NUTTX
CXXFLAGS += ${INCDIR_PREFIX}$(CURDIR)
CXXFLAGS += ${INCDIR_PREFIX}$(TOPDIR)/include/arch/board
# 将显式入口映射到Kconfig命令名称，保留原来的命令改名能力。
CXXFLAGS += -Drobot_main=$(subst ",,$(CONFIG_CBOARD_ROBOT_PROGNAME))_main
CXXFLAGS += -Dsbus_input_main=$(subst ",,$(CONFIG_CBOARD_SBUS_INPUT_PROGNAME))_main
CXXFLAGS += -Drc_update_main=$(subst ",,$(CONFIG_CBOARD_RC_UPDATE_PROGNAME))_main
CXXFLAGS += -Dparam_main=$(subst ",,$(CONFIG_CBOARD_PARAM_PROGNAME))_main
CXXFLAGS += -Dcontrol_allocator_main=$(subst ",,$(CONFIG_CBOARD_CONTROL_ALLOCATOR_PROGNAME))_main
CXXFLAGS += -Dcan_output_main=$(subst ",,$(CONFIG_CBOARD_CAN_OUTPUT_PROGNAME))_main
CXXFLAGS += -Dcommand_main=$(subst ",,$(CONFIG_CBOARD_COMMAND_PROGNAME))_main
CXXFLAGS += -Dfailsafe_main=$(subst ",,$(CONFIG_CBOARD_FAILSAFE_PROGNAME))_main
CXXFLAGS += -Drobot_control_main=$(subst ",,$(CONFIG_CBOARD_ROBOT_CONTROL_PROGNAME))_main
CXXFLAGS += -Dthickness_control_main=$(subst ",,$(CONFIG_CBOARD_THICKNESS_CONTROL_PROGNAME))_main
VPATH += :robot/common:robot/os:robot/orb:robot/drivers/rc:robot/modules:robot/params
VPATH += :robot/work_queue
VPATH += :robot/drivers/probe_lift
VPATH += :robot/modules/rc_update:robot/modules/command
VPATH += :robot/modules/failsafe:robot/modules/robot_control
CXXFLAGS += -Dcboard_link_main=$(subst ",,$(CONFIG_CBOARD_LINK_PROGNAME))_main
VPATH += :robot/modules/thickness_control:robot/modules/cboard_link
VPATH += :robot/modules/control_allocator
VPATH += :robot/modules/robot_runtime:robot/modules/sbus_input
VPATH += :robot/output/can_output
VPATH += :robot/configuration:robot/control:robot/output:protocol/can

include $(APPDIR)/Application.mk
