# STM32F103C8T6 - sensor firmware + SD logging (Windows GNU Make)
TARGET := firmware
BUILD_DIR := build
CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size
CPU := -mcpu=cortex-m3 -mthumb -mfloat-abi=soft
DEFS := -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER
KERNEL := middlewares/FreeRTOS-Kernel

INCS := -Iapp/inc \
        -Idrivers/inc \
        -Ibsp/inc \
        -Imiddlewares/FatFs \
        -Iconfig \
        -ILibraries/CMSIS/CM3/CoreSupport \
        -ILibraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x \
        -ILibraries/STM32F10x_StdPeriph_Driver/inc \
        -I$(KERNEL)/include \
        -I$(KERNEL)/portable/GCC/ARM_CM3

CFLAGS := $(CPU) -std=gnu11 -Os -g3 -ffunction-sections -fdata-sections -Wall -Wextra -MMD -MP $(DEFS) $(INCS)
LDFLAGS := $(CPU) -nostartfiles -Tlinker.ld -Wl,--gc-sections,-Map=$(BUILD_DIR)/$(TARGET).map -specs=nano.specs -specs=nosys.specs
LDFLAGS += -lm



# Sensor, interface and data pipeline from origin/develop
# SD, SPI, RTC and FatFS sources added from feature/sdcard-integration
CSRC := app/src/main.c \
        app/src/sensor_task.c \
        app/src/led_task.c \
        app/src/monitor_task.c \
        app/src/health_queue.c \
        app/src/sd_task.c \
        app/src/sd_rtc_sync.c \
        bsp/src/sd_rtc.c \
        drivers/src/i2c1.c \
        drivers/src/max30102.c \
        drivers/src/bpm.c \
        drivers/src/mlx90614.c \
        drivers/src/spo2.c \
        drivers/src/spi1.c \
        drivers/src/sd_spi.c \
        drivers/src/sd_diskio.c \
        middlewares/FatFs/ff.c \
        Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/system_stm32f10x.c \
        Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c \
        Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c \
        Libraries/STM32F10x_StdPeriph_Driver/src/stm32f10x_i2c.c \
        $(KERNEL)/tasks.c \
        $(KERNEL)/queue.c \
        $(KERNEL)/list.c \
        $(KERNEL)/portable/GCC/ARM_CM3/port.c \
        $(KERNEL)/portable/MemMang/heap_4.c

ASMSRC := Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x/startup/gcc_ride7/startup_stm32f10x_md.s
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(CSRC)) $(patsubst %.s,$(BUILD_DIR)/%.o,$(ASMSRC))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c
	@powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(dir $@)' | Out-Null"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.s
	@powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path '$(dir $@)' | Out-Null"
	$(CC) $(CPU) -x assembler-with-cpp -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) linker.ld Makefile
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

clean:
	@powershell -NoProfile -Command "if (Test-Path '$(BUILD_DIR)') { Remove-Item -Recurse -Force '$(BUILD_DIR)' }"

-include $(OBJS:.o=.d)
.PHONY: all clean
