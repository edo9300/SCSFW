# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Adrian "asie" Siekierka, 2023

export WONDERFUL_TOOLCHAIN ?= /opt/wonderful
TARGET = gba/multiboot
include $(WONDERFUL_TOOLCHAIN)/target/$(TARGET)/makedefs.mk
CXX		:= $(WONDERFUL_TOOLCHAIN)/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-g++

# Metadata
# --------

NAME		:= $(shell basename $(CURDIR))_mb

# Source code paths
# -----------------

INCLUDEDIRS	:= include
SOURCEDIRS	:= source
ASSETDIRS	:= assets
CBINDIRS	:= cbin

# Defines passed to all files
# ---------------------------

DEFINES		:=

# Libraries
# ---------

LIBS		:=
LIBDIRS		:=

# Build artifacts
# ---------------

BUILDDIR	:= build
ELF		:= build/$(NAME).elf
MAP		:= build/$(NAME).map
ROM		:= $(NAME).gba

# Verbose flag
# ------------

ifeq ($(V),1)
_V		:=
else
_V		:= @
endif

# Source files
# ------------

SOURCES_S	:= $(shell find -L $(SOURCEDIRS) -name "*.s")
SOURCES_C	:= $(shell find -L $(SOURCEDIRS) -name "*.c")
SOURCES_CXX	:= $(shell find -L $(SOURCEDIRS) -name "*.cpp")

# Compiler and linker flags
# -------------------------

WARNFLAGS	:= -Wall

INCLUDEFLAGS	:= $(foreach path,$(INCLUDEDIRS),-I$(path)) \
		   $(foreach path,$(LIBDIRS),-isystem $(path)/include)

LIBDIRSFLAGS	:= $(foreach path,$(LIBDIRS),-L$(path)/lib)

ASFLAGS		+= -x assembler-with-cpp $(DEFINES) $(WF_ARCH_CFLAGS) \
		   $(INCLUDEFLAGS) -ffunction-sections -fdata-sections -mthumb -mthumb-interwork

CFLAGS		+= -std=gnu2x $(WARNFLAGS) $(DEFINES) $(WF_ARCH_CFLAGS) \
		   $(INCLUDEFLAGS) -ffunction-sections -fdata-sections -mthumb -mthumb-interwork -Os

#workaround, only the scsd driver requires arm, due to inline asm, so force it as such

CXXFLAGS	+= $(WARNFLAGS) $(DEFINES) $(WF_ARCH_CFLAGS) \
		   $(INCLUDEFLAGS) -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -mthumb -mthumb-interwork -Os

LDFLAGS		:= $(LIBDIRSFLAGS) $(DEFINES) -Wl,-Map,$(MAP) -Wl,--gc-sections \
		   $(WF_ARCH_LDFLAGS) $(LIBS)

BUILDROMFLAGS	:=

# Intermediate build files
# ------------------------

OBJS_SOURCES	:= $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_S))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_C))) \
		   $(addsuffix .o,$(addprefix $(BUILDDIR)/,$(SOURCES_CXX)))

OBJS		:= $(OBJS_SOURCES)

DEPS		:= $(OBJS:.o=.d)

# Targets
# -------

.PHONY: all clean

all: $(ROM) compile_commands.json

$(ROM) $(ELF): $(OBJS)
	@echo "  ROM     $@"
	$(_V)$(ROMLINK) -o $(ROM) --output-elf $(ELF) $(ROMLINKFLAGS) -- $(OBJS) $(LDFLAGS)

clean:
	@echo "  CLEAN"
	$(_V)$(RM) $(ELF) $(ROM) $(BUILDDIR) compile_commands.json

compile_commands.json: $(OBJS) | Makefile
	@echo "  MERGE   compile_commands.json"
	$(_V)$(WF)/bin/wf-compile-commands-merge $@ $(patsubst %.o,%.cc.json,$^)

# Rules
# -----

$(BUILDDIR)/%.s.o : %.s | $(OBJS_ASSETS)
	@echo "  AS      $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(CC) $(ASFLAGS) -MMD -MP -MJ $(patsubst %.o,%.cc.json,$@) -c -o $@ $<

$(BUILDDIR)/%.c.o : %.c | $(OBJS_ASSETS)
	@echo "  CC      $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(CC) $(CFLAGS) -MMD -MP -MJ $(patsubst %.o,%.cc.json,$@) -c -o $@ $<

$(BUILDDIR)/%.cpp.o : %.cpp | $(OBJS_ASSETS)
	@echo "  CXX      $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(CXX) $(CXXFLAGS) -MMD -MP -MJ $(patsubst %.o,%.cc.json,$@) -c -o $@ $<

$(BUILDDIR)/%.bin.o $(BUILDDIR)/%_bin.h : %.bin
	@echo "  BIN2C   $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(WF)/bin/wf-bin2c -a 4 $(@D) $<
	$(_V)$(CC) $(CFLAGS) -MMD -MP -c -o $(BUILDDIR)/$*.bin.o $(BUILDDIR)/$*_bin.c

$(BUILDDIR)/%.lua.o : %.lua
	@echo "  PROCESS $<"
	@$(MKDIR) -p $(@D)
	$(_V)$(WF)/bin/wf-process -o $(BUILDDIR)/$*.c -t $(TARGET) --depfile $(BUILDDIR)/$*.lua.d --depfile-target $(BUILDDIR)/$*.lua.o $<
	$(_V)$(CC) $(CFLAGS) -c -o $(BUILDDIR)/$*.lua.o $(BUILDDIR)/$*.c

# Include dependency files if they exist
# --------------------------------------

-include $(DEPS)
