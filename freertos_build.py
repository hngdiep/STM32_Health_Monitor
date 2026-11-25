
Import("env")

from os.path import join

PROJECT = env.subst("$PROJECT_DIR")

# Include paths for all C sources
env.Append(
    CPPPATH=[
        join(PROJECT, "config"),
        join(PROJECT, "app", "inc"),
        join(PROJECT, "drivers", "inc"),
        join(PROJECT, "bsp", "inc"),
        join(PROJECT, "middlewares", "FatFs"),
        join(PROJECT, "Libraries", "CMSIS", "CM3", "CoreSupport"),
        join(
            PROJECT, "Libraries", "CMSIS", "CM3",
            "DeviceSupport", "ST", "STM32F10x"
        ),
        join(
            PROJECT, "Libraries",
            "STM32F10x_StdPeriph_Driver", "inc"
        ),
        join(
            PROJECT, "middlewares",
            "FreeRTOS-Kernel", "include"
        ),
        join(
            PROJECT, "middlewares", "FreeRTOS-Kernel",
            "portable", "GCC", "ARM_CM3"
        ),
    ]
)

# FreeRTOS Kernel
env.BuildSources(
    "$BUILD_DIR/freertos",
    "$PROJECT_DIR/middlewares/FreeRTOS-Kernel",
    src_filter=[
        "+<tasks.c>",
        "+<queue.c>",
        "+<list.c>",
        "+<portable/GCC/ARM_CM3/port.c>",
        "+<portable/MemMang/heap_4.c>",
    ],
)

# STM32 SPL drivers
env.BuildSources(
    "$BUILD_DIR/spl",
    "$PROJECT_DIR/Libraries/STM32F10x_StdPeriph_Driver/src",
    src_filter=[
        "+<stm32f10x_gpio.c>",
        "+<stm32f10x_rcc.c>",
        "+<stm32f10x_spi.c>",
        "+<stm32f10x_i2c.c>",
    ],
)

# TFT, sensor and MicroSD drivers
env.BuildSources(
    "$BUILD_DIR/drivers",
    "$PROJECT_DIR/drivers/src",
    src_filter=[
        "+<tft_spi2.c>",
        "+<tft_display.c>",
        "+<i2c1.c>",
        "+<max30102.c>",
        "+<mlx90614.c>",
        "+<bpm.c>",
        "+<spo2.c>",
        "+<spi1.c>",
        "+<sd_spi.c>",
        "+<sd_diskio.c>",
    ],
)

# RTC driver
env.BuildSources(
    "$BUILD_DIR/bsp",
    "$PROJECT_DIR/bsp/src",
    src_filter=[
        "+<sd_rtc.c>",
    ],
)

# FatFS
env.BuildSources(
    "$BUILD_DIR/fatfs",
    "$PROJECT_DIR/middlewares/FatFs",
    src_filter=[
        "+<ff.c>",
    ],
)
