#include "spi1.h"
#include "stm32f1xx.h"

/* Finite polling avoids hard locks if wiring/clocking fails. */
#define SPI_POLL_LIMIT 200000

static bool spi_wait(uint32_t mask, bool set)
{
    for (uint32_t n = 0; n < SPI_POLL_LIMIT; n++) {
        if (((SPI1->SR & mask) != 0) == set) {
            return true;
        }
    }
    return false;
}

void SPI1_CS_High(void) { GPIOA->BSRR = GPIO_BSRR_BS4; }
void SPI1_CS_Low(void)  { GPIOA->BRR = GPIO_BRR_BR4; }

void SPI1_SetSlow(void)
{
    if (!SPI1_Ready()) return;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1 = (SPI1->CR1 & ~SPI_CR1_BR) |
                SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

void SPI1_SetFast(void)
{
    if (!SPI1_Ready()) return;
    SPI1->CR1 &= ~SPI_CR1_SPE;
    /* BR[2:0]=011 => APB2/16. */
    SPI1->CR1 = (SPI1->CR1 & ~SPI_CR1_BR) | SPI_CR1_BR_1 | SPI_CR1_BR_0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

bool SPI1_Ready(void)
{
    return spi_wait(SPI_SR_TXE, true) && spi_wait(SPI_SR_BSY, false);
}

void SPI1_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN | RCC_APB2ENR_AFIOEN;

    /* PA4: general purpose push-pull output 50 MHz (0x3).
       PA5/PA7: AF push-pull output 50 MHz (0xB).
       PA6: floating input (0x4). */
    GPIOA->CRL &= ~((0xF << 16) | (0xF << 20) | (0xF << 24) | (0xF << 28));
    GPIOA->CRL |= (0x3 << 16) | (0xB << 20) | (0x4 << 24) | (0xB << 28);

    SPI1_CS_High();
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
                SPI_CR1_BR_2 | SPI_CR1_BR_1 | SPI_CR1_BR_0;
    SPI1->CR2 = 0;
    SPI1->CR1 |= SPI_CR1_SPE;
}

bool SPI1_Transfer(uint8_t tx, uint8_t *rx)
{
    if (!spi_wait(SPI_SR_TXE, true)) return false;
    *(__IO uint8_t *)&SPI1->DR = tx;
    if (!spi_wait(SPI_SR_RXNE, true)) return false;
    uint8_t received = *(__IO uint8_t *)&SPI1->DR;
    if (!spi_wait(SPI_SR_BSY, false)) return false;
    if (rx != 0) *rx = received;
    return true;
}