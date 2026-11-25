
#ifndef TFT_SPI2_H
#define TFT_SPI2_H

#include <stdint.h>

void TFT_SPI2_Init(void);
void TFT_Reset(void);

void TFT_Select(void);
void TFT_Deselect(void);

void TFT_WriteCommand(uint8_t cmd);
void TFT_WriteData(uint8_t data);

#endif
