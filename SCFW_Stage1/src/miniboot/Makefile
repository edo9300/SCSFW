# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Adrian "asie" Siekierka, 2024

export WONDERFUL_TOOLCHAIN ?= /opt/wonderful
export BLOCKSDS ?= /opt/blocksds/core

# Tools
# -----

LUA		:= $(WONDERFUL_TOOLCHAIN)/bin/wf-lua
DLDIPATCH	:= $(BLOCKSDS)/tools/dldipatch/dldipatch
NDSTOOL		:= $(BLOCKSDS)/tools/ndstool/ndstool
CC		:= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-gcc
OBJCOPY		:= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-objcopy
CP		:= cp
MAKE		:= make
MKDIR		:= mkdir
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

ARM9ELF		:= build/arm9.elf
ARM7ELF		:= build/arm7.elf

.PHONY: all clean arm9 arm7

all: arm9 arm7
	@$(MKDIR) -p dist/
	$(_V)$(CP) LICENSE README.md dist/


clean:
	@echo "  CLEAN"
	$(_V)$(RM) build dist	

arm9:
	$(_V)+$(MAKE) -f Makefile.miniboot TARGET=arm9 --no-print-directory

arm7:
	$(_V)+$(MAKE) -f Makefile.miniboot TARGET=arm7 --no-print-directory
