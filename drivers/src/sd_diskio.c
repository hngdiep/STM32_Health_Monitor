#include "ff.h"
#include "diskio.h"
#include <stddef.h>
#include "sd_spi.h"
#include <stdint.h>

/* FatFS R0.15 / R0.15b: LBA_t sector addressing. Only physical drive 0. */
DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return SD_SPI_Init() ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return SD_SPI_Initialized() ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || buff == 0 || count == 0) return RES_PARERR;
    if (!SD_SPI_Initialized()) return RES_NOTRDY;
    if (sector > UINT32_MAX || (uint64_t)sector + count > (uint64_t)UINT32_MAX + 1)
        return RES_PARERR;
    for (UINT n = 0; n < count; n++) {
        if (!SD_SPI_ReadSector((uint32_t)sector + n,
                               buff + (size_t)n * SD_SECTOR_SIZE)) return RES_ERROR;
    }
    return RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
    if (pdrv != 0 || buff == 0 || count == 0) return RES_PARERR;
    if (!SD_SPI_Initialized()) return RES_NOTRDY;
    if (sector > UINT32_MAX || (uint64_t)sector + count > (uint64_t)UINT32_MAX + 1)
        return RES_PARERR;
    for (UINT n = 0; n < count; n++) {
        if (!SD_SPI_WriteSector((uint32_t)sector + n,
                                buff + (size_t)n * SD_SECTOR_SIZE)) return RES_ERROR;
    }
    return RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) return RES_PARERR;
    if (!SD_SPI_Initialized()) return RES_NOTRDY;
    switch (cmd) {
        case CTRL_SYNC:
            return SD_SPI_Sync() ? RES_OK : RES_ERROR;
        case GET_SECTOR_COUNT: {
            uint32_t sectors = 0;
            if (buff == 0 || !SD_SPI_GetSectorCount(&sectors)) return RES_ERROR;
            *(DWORD *)buff = (DWORD)sectors;
            return RES_OK;
        }
        case GET_SECTOR_SIZE:
            if (buff == 0) return RES_PARERR;
            *(WORD *)buff = SD_SECTOR_SIZE;
            return RES_OK;
        case GET_BLOCK_SIZE:
            /* Conservative FAT block erase granularity, in 512-byte sectors.
               FF_USE_MKFS=0: not used by logger. */
            if (buff == 0) return RES_PARERR;
            *(DWORD *)buff = 1;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}