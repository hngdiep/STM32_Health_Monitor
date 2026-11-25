
#ifndef SD_LOGGER_BRIDGE_H
#define SD_LOGGER_BRIDGE_H

#include <stdint.h>

/* 0 = chua chon nguoi dung, khong ghi SD */
void SD_Logger_SetUser(uint8_t user_id);
uint8_t SD_Logger_GetUser(void);

void SD_Logger_Bridge_Task(void *argument);

#endif
