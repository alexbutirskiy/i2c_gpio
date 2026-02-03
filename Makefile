
BUILD := _build

CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
CPP := $(CROSS_COMPILE)g++
AR := $(CROSS_COMPILE)ar
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE := $(CROSS_COMPILE)size
AS := $(CROSS_COMPILE)as

CFLAGS := -Wall -Wextra -O2 -g -DSTM32G031xx -mcpu=cortex-m0plus -mthumb -ffunction-sections -fdata-sections

LDFLAGS := -T linker.ld

INCLUDE := -Isrc -Ilib -Idrivers/cmsis_device_g0/Include/

SRCS := $(shell find src lib -type f \( -name '*.c' -o -name '*.s' \))
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))
OBJS := $(patsubst %.s,$(BUILD)/%.o,$(OBJS))

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $< -o $@

OUT := $(BUILD)/$(notdir $(CURDIR)).elf

all: $(OUT)

$(OUT): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@

debug:
	@echo $(OBJS)