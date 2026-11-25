
#include "tft_spi2.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"

#include "FreeRTOS.h"
#include "task.h"

#define TFT_CS_PIN   GPIO_Pin_12
#define TFT_DC_PIN   GPIO_Pin_0
#define TFT_RST_PIN  GPIO_Pin_1

#define SPI_TIMEOUT  1000000UL

void TFT_SPI2_Init(void)
{
    GPIO_InitTypeDef gpio;
    SPI_InitTypeDef spi;

    // Enable GPIOB and SPI2 clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    // PB13 = SPI2_SCK
    // PB15 = SPI2_MOSI
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    // PB14 = SPI2_MISO
    gpio.GPIO_Pin = GPIO_Pin_14;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &gpio);

    // PB12 = TFT_CS
    gpio.GPIO_Pin = TFT_CS_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    // PB0 = TFT_DC
    // PB1 = TFT_RESET
    gpio.GPIO_Pin = TFT_DC_PIN | TFT_RST_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    // Initial GPIO levels
    GPIO_SetBits(GPIOB, TFT_CS_PIN | TFT_RST_PIN);
    GPIO_ResetBits(GPIOB, TFT_DC_PIN);

    // Configure SPI2 master, mode 0
    SPI_I2S_DeInit(SPI2);
    SPI_StructInit(&spi);

    spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi.SPI_Mode = SPI_Mode_Master;
    spi.SPI_DataSize = SPI_DataSize_8b;
    spi.SPI_CPOL = SPI_CPOL_Low;
    spi.SPI_CPHA = SPI_CPHA_1Edge;
    spi.SPI_NSS = SPI_NSS_Soft;

    // Use a low SPI clock for initial testing
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32;

    spi.SPI_FirstBit = SPI_FirstBit_MSB;

    SPI_Init(SPI2, &spi);

    // IMPORTANT:
    // Set internal NSS high when software NSS is used.
    // Otherwise SPI master may encounter MODF.
    SPI_NSSInternalSoftwareConfig(
        SPI2,
        SPI_NSSInternalSoft_Set
    );

    // Enable SPI2
    SPI_Cmd(SPI2, ENABLE);
}

static void TFT_SendByte(uint8_t data)
{
    uint32_t timeout;

    timeout = SPI_TIMEOUT;

    while (SPI_I2S_GetFlagStatus(
        SPI2, SPI_I2S_FLAG_TXE) == RESET)
    {
        if (--timeout == 0)
            return;
    }

    SPI_I2S_SendData(SPI2, data);

    timeout = SPI_TIMEOUT;

    while (SPI_I2S_GetFlagStatus(
        SPI2, SPI_I2S_FLAG_RXNE) == RESET)
    {
        if (--timeout == 0)
            return;
    }

    // Discard unused received data
    (void)SPI_I2S_ReceiveData(SPI2);

    timeout = SPI_TIMEOUT;

    while (SPI_I2S_GetFlagStatus(
        SPI2, SPI_I2S_FLAG_BSY) == SET)
    {
        if (--timeout == 0)
            return;
    }
}

void TFT_Select(void)
{
    GPIO_ResetBits(GPIOB, TFT_CS_PIN);
}

void TFT_Deselect(void)
{
    GPIO_SetBits(GPIOB, TFT_CS_PIN);
}

void TFT_WriteCommand(uint8_t cmd)
{
    GPIO_ResetBits(GPIOB, TFT_DC_PIN);
    TFT_SendByte(cmd);
}

void TFT_WriteData(uint8_t data)
{
    GPIO_SetBits(GPIOB, TFT_DC_PIN);
    TFT_SendByte(data);
}

void TFT_Reset(void)
{
    // Must be called from a running FreeRTOS task
    GPIO_ResetBits(GPIOB, TFT_RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(20));

    GPIO_SetBits(GPIOB, TFT_RST_PIN);
    vTaskDelay(pdMS_TO_TICKS(150));
}
