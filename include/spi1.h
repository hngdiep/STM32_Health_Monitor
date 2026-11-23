#ifndef SPI1_H
#define SPI1_H

#include <stdbool.h>
#include <stdint.h>

/* SPI1: PA4 = CS, PA5 = SCK, PA6 = MISO, PA7 = MOSI. */
void SPI1_Init(void);
void SPI1_SetSlow(void);  /* APB2 / 256: < 400 kHz with APB2 <= 72 MHz */
void SPI1_SetFast(void);  /* APB2 / 16: 4.5 MHz at APB2 = 72 MHz */
void SPI1_CS_Low(void);
void SPI1_CS_High(void);
bool SPI1_Transfer(uint8_t tx, uint8_t *rx);
bool SPI1_Ready(void);

#endif