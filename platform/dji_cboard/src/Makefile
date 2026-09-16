# SPDX-License-Identifier: Apache-2.0

include $(TOPDIR)/Make.defs
include Make.defs

DEPPATH += --dep-path .
VPATH += :.
CFLAGS += ${INCDIR_PREFIX}.

STM32_COMMON_SRCDIR := $(TOPDIR)$(DELIM)arch$(DELIM)$(CONFIG_ARCH)$(DELIM)src$(DELIM)common$(DELIM)stm32
CFLAGS += ${INCDIR_PREFIX}$(STM32_COMMON_SRCDIR)

include $(TOPDIR)/boards/Board.mk
