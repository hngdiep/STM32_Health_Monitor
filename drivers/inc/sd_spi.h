#ifndef SD_SPI_H
#define SD_SPI_H

#include <stdbool.h>
#include <stdint.h>

#define SD_SECTOR_SIZE 512

bool SD_SPI_Init(void);
bool SD_SPI_Initialized(void);
void SD_SPI_Invalidate(void);
bool SD_SPI_ReadSector(uint32_t lba, uint8_t *buffer);
bool SD_SPI_WriteSector(uint32_t lba, const uint8_t *buffer);
bool SD_SPI_GetSectorCount(uint32_t *sector_count);
bool SD_SPI_Sync(void);

#endif