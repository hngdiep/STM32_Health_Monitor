
#include "sd_logger_bridge.h"
#include "sd_task.h"

/*
 * Luu User ID hien tai.
 *
 * 0 = chua chon nguoi dung.
 */
static volatile uint8_t g_selected_user = 0U;

/*
 * Cap nhat User ID.
 * Chi chap nhan cac gia tri 0..10.
 */
void SD_Logger_SetUser(uint8_t user_id)
{
    if (user_id <= SD_LOG_MAX_USERS)
    {
        g_selected_user = user_id;
    }
}

/*
 * Lay User ID hien tai de hien thi tren TFT.
 */
uint8_t SD_Logger_GetUser(void)
{
    return g_selected_user;
}
