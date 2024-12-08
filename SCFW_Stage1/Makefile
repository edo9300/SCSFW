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
CC		:= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-gcc
CP		:= cp
MAKE	:= make
MKDIR	:= mkdir
DD		:= dd
CAT		:= cat
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
	$(_V)$(CP) build/firmware-rumble.frm firmware-rumble.frm

clean:
	@echo "  CLEAN"
	$(_V)$(RM) build src/miniboot/build src/miniboot/dist firmware.frm firmware-rumble.frm

stage1: miniboot set_backlight
	@$(MKDIR) -p build
	$(_V)$(AS) -I src src/scsfw.s -o build/scsfw.out
	$(_V)$(OBJCOPY) -O binary build/scsfw.out build/firmware.frm
	$(_V)$(DD) if=/dev/zero bs=1 count=262144 > build/padding-rumble.bin
	$(_V)$(CAT) build/padding-rumble.bin build/firmware.frm > build/firmware-rumble.frm
	$(_V)$(GBATOOL) fix -l official build/firmware.frm
	$(_V)$(GBATOOL) fix -l official build/firmware-rumble.frm

set_backlight:
	@$(MKDIR) -p build
	$(_V)$(CC) -std=gnu23 -nostartfiles -nostdlib -T src/backlight.ld -ffreestanding -ffunction-sections -fpic -fPIC -O3 src/set_backlight.c -o build/set_backlight.o
	$(_V)$(OBJCOPY) -O binary -j .text build/set_backlight.o build/set_backlight.bin

miniboot:
	$(_V)+$(MAKE) -C src/miniboot --no-print-directory
