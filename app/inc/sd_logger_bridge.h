
#ifndef SD_LOGGER_BRIDGE_H
#define SD_LOGGER_BRIDGE_H

#include <stdint.h>

/*
 * Quan ly User ID duoc chon tren giao dien.
 *
 * 0     : Khong chon nguoi dung
 * 1..10 : ID nguoi dung
 */

void SD_Logger_SetUser(uint8_t user_id);

uint8_t SD_Logger_GetUser(void);

#endif
