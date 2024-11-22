# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Adrian "asie" Siekierka, 2024

export WONDERFUL_TOOLCHAIN ?= /opt/wonderful
export BLOCKSDS ?= /opt/blocksds/core

# Tools
# -----

GBATOOL	:= $(WONDERFUL_TOOLCHAIN)/bin/wf-gbatool
OBJCOPY	:= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-objcopy
AS		:= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-as
CP		:= cp
MAKE	:= make
MKDIR	:= mkdir
RM		:= rm -rf

# Verbose flag
# ------------

ifeq ($(V),1)
_V		:=
else
_V		:= @
endif

# Build rules
# -----------

.PHONY: all clean miniboot stage1

all: stage1
	$(_V)$(CP) build/firmware.frm firmware.frm

clean:
	@echo "  CLEAN"
	$(_V)$(RM) build miniboot/build miniboot/dist firmware.frm

stage1: miniboot
	@$(MKDIR) -p build
	$(_V)$(AS) scsfw.s -o build/scsfw.out
	$(_V)$(OBJCOPY) -O binary build/scsfw.out build/firmware.frm
	$(_V)$(GBATOOL) fix -l official build/firmware.frm

miniboot:
	$(_V)+$(MAKE) -C miniboot --no-print-directory
