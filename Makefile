
BUILD := _build

CROSS_COMPILE ?= arm-none-eabi-

CC := $(CROSS_COMPILE)gcc
CPP := $(CROSS_COMPILE)g++
AR := $(CROSS_COMPILE)ar
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump
SIZE := $(CROSS_COMPILE)size
AS := $(CROSS_COMPILE)as


CFLAGS := -std=c11 -Wall -Wextra -O2 -g -DSTM32G030xx -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft -ffunction-sections -fdata-sections
ASFLAGS := -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft

LDFLAGS := -Wl,--no-warn-mismatch -mcpu=cortex-m0plus -mthumb -mfloat-abi=soft \
--specs=nano.specs --specs=nosys.specs -Wl,--gc-sections  \
-L lib/ld -T stm32g030x6.ld

INC_DIRS := src lib drivers/cmsis_device_g0/Include drivers/CMSIS_6/CMSIS/Core/Include \
  drivers/stm32g0xx-hal/Inc lib/i2c_gpio lib/i2cDev lib/timer
INCLUDE := $(foreach dir,$(INC_DIRS),-I$(dir))

SRC_DIRS := src lib
SRCS := $(shell find $(SRC_DIRS) -type f \( -name '*.c' -o -name '*.s' \))
OBJS := $(patsubst %.c,$(BUILD)/%.o,$(SRCS))
OBJS := $(patsubst %.s,$(BUILD)/%.o,$(OBJS))

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(INCLUDE) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

OUT := $(BUILD)/$(notdir $(CURDIR)).elf

all: $(OUT)

$(OUT): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)
	$(SIZE) $@
	$(OBJCOPY) -O ihex $@ $(patsubst %.elf,%.hex,$@)

clean:
	rm -rf $(BUILD)

spec:
	@echo "Preprocessor output for debugging:"
	$(CC) $(INCLUDE) $(CFLAGS) -E -P -v -dD "${INPUTS}"

debug:
	@echo $(OBJS)