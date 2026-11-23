
#include "health_ui.h"
#include "health_data.h"
#include "health_controls.h"
#include "tft_display.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define BG 0x0000U
#define FG 0xFFFFU

// Chi ve lai dong co noi dung thay doi
static void show_line(int y, const char *s, char *old)
{
    if (strcmp(s, old) == 0) return;
    TFT_FillRect(10, y, 220, 20, BG);
    TFT_DrawText(10, y, s, FG, BG);
    strcpy(old, s);
}

void Health_UI_Task(void *argument)
{
    HealthReading r;
    HealthLimits lim;
    uint8_t mode, warning;
    char s[32];
    char old[5][32] = {{0}};

    (void)argument;

    TFT_Display_Init();
    TFT_FillScreen(BG);
    TFT_DrawText(10, 15, "HEALTH MONITOR", FG, BG);

    for (;;)
    {
        r = HealthData_Get();
        lim = HealthLimits_Get();
        mode = HealthControls_GetMode();

        warning = r.valid &&
            (r.bpm > lim.bpm_max ||
             r.spo2 < lim.spo2_min ||
             r.temp_x10 > lim.temp_max_x10);

        // Nhip tim
        if (r.valid)
            snprintf(s, sizeof(s), "BPM: %u", (unsigned)r.bpm);
        else
            strcpy(s, "BPM: --");
        show_line(60, s, old[0]);

        // Oxy mau
        if (r.valid)
            snprintf(s, sizeof(s), "SPO2: %u%%", (unsigned)r.spo2);
        else
            strcpy(s, "SPO2: --");
        show_line(95, s, old[1]);

        // Nhiet do
        if (r.valid)
            snprintf(s, sizeof(s), "TEMP: %d.%d C",
                (int)(r.temp_x10 / 10),
                (int)(r.temp_x10 < 0 ?
                (-r.temp_x10) % 10 : r.temp_x10 % 10));
        else
            strcpy(s, "TEMP: --");
        show_line(130, s, old[2]);

        // Trang thai
        if (!r.valid) strcpy(s, "STATUS: WAITING");
        else if (warning) strcpy(s, "STATUS: WARNING");
        else strcpy(s, "STATUS: NORMAL");
        show_line(180, s, old[3]);

        // Cai dat nguong
        if (mode == 0) strcpy(s, "SET: SETTINGS");
        else if (mode == 1)
            snprintf(s, sizeof(s), "BPM MAX: %u",
                     (unsigned)lim.bpm_max);
        else if (mode == 2)
            snprintf(s, sizeof(s), "SPO2 MIN: %u",
                     (unsigned)lim.spo2_min);
        else
            snprintf(s, sizeof(s), "TEMP MAX: %d C",
                     (int)(lim.temp_max_x10 / 10));
        show_line(235, s, old[4]);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
