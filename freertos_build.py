
Import("env")

# Compile FreeRTOS Kernel
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

# Compile SPL GPIO and RCC drivers
env.BuildSources(
    "$BUILD_DIR/spl",
    "$PROJECT_DIR/Libraries/STM32F10x_StdPeriph_Driver/src",
    src_filter=[
        "+<stm32f10x_gpio.c>",
        "+<stm32f10x_rcc.c>",
    ],
)

# Compile SPL SPI driver
env.BuildSources(
    "$BUILD_DIR/spl_spi",
    "$PROJECT_DIR/Libraries/STM32F10x_StdPeriph_Driver/src",
    src_filter=[
        "+<stm32f10x_spi.c>",
    ],
)

# Compile TFT SPI2 driver
env.BuildSources(
    "$BUILD_DIR/tft",
    "$PROJECT_DIR/drivers/src",
    src_filter=[
        "+<tft_spi2.c>",
        "+<tft_display.c>",
    ],
)
