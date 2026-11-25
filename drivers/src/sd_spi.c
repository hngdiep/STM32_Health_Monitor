#include "sd_spi.h"
#include "spi1.h"
#include "FreeRTOS.h"
#include "task.h"

/* SD native command numbers in SPI mode. */
#define SD_CMD0     0
#define SD_CMD8     8
#define SD_CMD9     9
#define SD_CMD16   16
#define SD_CMD17   17
#define SD_CMD24   24
#define SD_CMD55   55
#define SD_CMD58   58
#define SD_ACMD41  41
#define SD_TIMEOUT_TICKS pdMS_TO_TICKS(500)

static bool card_ready;
static bool card_sdhc;

/* Execute this driver from Logging_Task only. No SPI bus mutex is required
   as long as no other Task accesses SPI1. */
static bool xfer(uint8_t value, uint8_t *reply)
{
    return SPI1_Transfer(value, reply);
}

static void deselect_card(void)
{
    uint8_t ignored;
    SPI1_CS_High();
    (void)xfer(0xFF, &ignored);
}

static bool select_card(void)
{
    SPI1_CS_Low();
    uint8_t r;
    if (!xfer(0xFF, &r)) return false;
    return true;
}

static bool wait_ready(TickType_t timeout)
{
    TickType_t start = xTaskGetTickCount();
    uint8_t response;
    do {
        if (!xfer(0xFF, &response)) return false;
        if (response == 0xFF) return true;
        vTaskDelay(1);
    } while ((xTaskGetTickCount() - start) < timeout);
    return false;
}

/* Send command with CS already asserted; keep CS asserted for data phase. */
static bool command(uint8_t cmd, uint32_t arg, uint8_t *r1)
{
    uint8_t ignored;
    uint8_t crc = 0x01;
    if (cmd == SD_CMD0) crc = 0x95;
    if (cmd == SD_CMD8) crc = 0x87;

    if (!wait_ready(SD_TIMEOUT_TICKS)) return false;
    if (!xfer((uint8_t)(0x40 | cmd), &ignored)) return false;
    if (!xfer((uint8_t)(arg >> 24), &ignored)) return false;
    if (!xfer((uint8_t)(arg >> 16), &ignored)) return false;
    if (!xfer((uint8_t)(arg >> 8), &ignored)) return false;
    if (!xfer((uint8_t)arg, &ignored)) return false;
    if (!xfer(crc, &ignored)) return false;

    for (int n = 0; n < 10; n++) {
        if (!xfer(0xFF, r1)) return false;
        if ((*r1 & 0x80) == 0) return true;
    }
    return false;
}

static bool command_transaction(uint8_t cmd, uint32_t arg, uint8_t *r1)
{
    deselect_card();
    if (!select_card()) return false;
    return command(cmd, arg, r1);
}


static bool read_data(uint8_t *dst, uint16_t size)
{
    TickType_t start = xTaskGetTickCount();
    uint8_t token = 0xFF;

    do {
        if (!xfer(0xFF, &token))
            return false;

        if (token == 0xFE)
            break;

        if (token != 0xFF)
            return false;

    } while ((xTaskGetTickCount() - start) < SD_TIMEOUT_TICKS);

    if (token != 0xFE)
        return false;

    // Doc du lieu sector
    for (uint16_t i = 0; i < size; i++) {
        if (!xfer(0xFF, &dst[i]))
            return false;
    }

    // Bo qua 2 byte CRC16
    uint8_t ignored;

    return xfer(0xFF, &ignored) &&
           xfer(0xFF, &ignored);
}


void SD_SPI_Invalidate(void)
{
    card_ready = false;
    card_sdhc = false;
    SPI1_SetSlow();
    deselect_card();
}

bool SD_SPI_Initialized(void) { return card_ready; }

bool SD_SPI_Init(void)
{
    card_ready = false;
    card_sdhc = false;
    SPI1_Init();
    SPI1_SetSlow();
    deselect_card();

    uint8_t ignored;
    for (int i = 0; i < 10; i++) {
        if (!xfer(0xFF, &ignored)) return false; /* >= 80 clocks, CS high. */
    }

    uint8_t r1 = 0xFF;
    bool entered_idle = false;
    for (int tries = 0; tries < 10; tries++) {
        if (command_transaction(SD_CMD0, 0, &r1) && r1 == 0x01) {
            entered_idle = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (!entered_idle) goto fail;

    bool v2 = false;
    if (!command_transaction(SD_CMD8, 0x1AA, &r1)) goto fail;
    if (r1 == 0x01) {
        uint8_t r7[4];
        for (int i = 0; i < 4; i++) {
            if (!xfer(0xFF, &r7[i])) goto fail;
        }
        if (r7[2] != 0x01 || r7[3] != 0xAA) goto fail;
        v2 = true;
    } else if ((r1 & 0x04) == 0) {
        goto fail;
    }

    TickType_t start = xTaskGetTickCount();
    bool activated = false;
    do {
        if (!command_transaction(SD_CMD55, 0, &r1) || r1 > 0x01) goto fail;
        if (!command(SD_ACMD41, v2 ? 0x40000000 : 0, &r1)) goto fail;
        if (r1 == 0) {
            activated = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    } while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(5000));
    if (!activated) goto fail;

    if (v2) {
        if (!command_transaction(SD_CMD58, 0, &r1) || r1 != 0) goto fail;
        uint8_t ocr[4];
        for (int i = 0; i < 4; i++) {
            if (!xfer(0xFF, &ocr[i])) goto fail;
        }
        card_sdhc = (ocr[0] & 0x40) != 0;
    }
    if (!card_sdhc) {
        if (!command_transaction(SD_CMD16, SD_SECTOR_SIZE, &r1) || r1 != 0) goto fail;
    }
    deselect_card();
    SPI1_SetSlow();
    card_ready = true;
    return true;

fail:
    deselect_card();
    return false;
}

static bool sector_address(uint32_t sector, uint32_t *address)
{
    if (!card_ready) return false;
    if (!card_sdhc && sector > (UINT32_MAX / SD_SECTOR_SIZE)) return false;
    *address = card_sdhc ? sector : sector * SD_SECTOR_SIZE;
    return true;
}

bool SD_SPI_ReadSector(uint32_t lba, uint8_t *buffer)
{
    if (buffer == 0) return false;
    uint32_t arg;
    uint8_t r1;
    if (!sector_address(lba, &arg)) return false;
    bool ok = command_transaction(SD_CMD17, arg, &r1) && r1 == 0 &&
              read_data(buffer, SD_SECTOR_SIZE);
    deselect_card();
    if (!ok) SD_SPI_Invalidate();
    return ok;
}

bool SD_SPI_WriteSector(uint32_t lba, const uint8_t *buffer)
{
    if (buffer == 0) return false;
    uint32_t arg;
    uint8_t r1, ignored, reply;
    if (!sector_address(lba, &arg)) return false;
    bool ok = command_transaction(SD_CMD24, arg, &r1) && r1 == 0;
    if (ok) ok = xfer(0xFF, &ignored) && xfer(0xFE, &ignored);
    for (int i = 0; ok && i < SD_SECTOR_SIZE; i++) {
        ok = xfer(buffer[i], &ignored);
    }
    if (ok) ok = xfer(0xFF, &ignored) && xfer(0xFF, &ignored);
    if (ok) ok = xfer(0xFF, &reply) && ((reply & 0x1F) == 0x05);
    if (ok) ok = wait_ready(pdMS_TO_TICKS(1000));
    deselect_card();
    if (!ok) SD_SPI_Invalidate();
    return ok;
}

bool SD_SPI_GetSectorCount(uint32_t *sector_count)
{
    if (!card_ready || sector_count == 0) return false;
    uint8_t csd[16], r1;
    bool ok = command_transaction(SD_CMD9, 0, &r1) && r1 == 0 &&
              read_data(csd, sizeof(csd));
    deselect_card();
    if (!ok) return false;

    if ((csd[0] >> 6) == 1) {
        uint32_t c_size = ((uint32_t)(csd[7] & 0x3F) << 16) |
                          ((uint32_t)csd[8] << 8) | csd[9];
        uint64_t sectors = ((uint64_t)c_size + 1) * 1024;
        if (sectors > UINT32_MAX) return false;
        *sector_count = (uint32_t)sectors;
    } else if ((csd[0] >> 6) == 0) {
        uint32_t c_size = ((uint32_t)(csd[6] & 0x03) << 10) |
                          ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
        uint32_t c_mult = ((uint32_t)(csd[9] & 0x03) << 1) | (csd[10] >> 7);
        uint32_t read_bl_len = csd[5] & 0x0F;
        uint64_t bytes = ((uint64_t)c_size + 1) << (c_mult + 2 + read_bl_len);
        uint64_t sectors = bytes / SD_SECTOR_SIZE;
        if (sectors == 0 || sectors > UINT32_MAX) return false;
        *sector_count = (uint32_t)sectors;
    } else {
        return false;
    }
    return true;
}

bool SD_SPI_Sync(void)
{
    if (!card_ready) return false;
    deselect_card();
    if (!select_card()) return false;
    bool ok = wait_ready(pdMS_TO_TICKS(1000));
    deselect_card();
    if (!ok) SD_SPI_Invalidate();
    return ok;
}