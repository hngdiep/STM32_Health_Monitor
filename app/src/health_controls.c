
#include "health_controls.h"
#include "health_data.h"
#include "sd_logger_bridge.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>

/* Nut nhan PA0, PA1, PA2 */
#define BTN_SET GPIO_Pin_0
#define BTN_UP  GPIO_Pin_1
#define BTN_DN  GPIO_Pin_2

/* LED va Buzzer canh bao */
#define WARN_LED  GPIO_Pin_8
#define WARN_BUZZ GPIO_Pin_9

/*
 * Che do cai dat:
 * 0 - SETTINGS
 * 1 - BPM MAX
 * 2 - SPO2 MIN
 * 3 - TEMP MAX
 * 4 - USER ID
 */
static volatile uint8_t g_mode = 0;

/* Lay che do dang chon */
uint8_t HealthControls_GetMode(void)
{
    return g_mode;
}

/* Khoi tao cac nut nhan */
static void buttons_init(void)
{
    GPIO_InitTypeDef g;

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA,
        ENABLE
    );

    GPIO_StructInit(&g);

    g.GPIO_Pin = BTN_SET | BTN_UP | BTN_DN;
    g.GPIO_Mode = GPIO_Mode_IPU;

    GPIO_Init(GPIOA, &g);
}

/* Dieu chinh nguong va USER ID */
static void adjust(int8_t dir)
{
    HealthLimits l = HealthLimits_Get();

    switch (g_mode)
    {
        /* BPM MAX */
        case 1:
            if (dir > 0 && l.bpm_max < 220)
            {
                l.bpm_max++;
            }

            if (dir < 0 && l.bpm_max > 40)
            {
                l.bpm_max--;
            }
            break;

        /* SPO2 MIN */
        case 2:
            if (dir > 0 && l.spo2_min < 100)
            {
                l.spo2_min++;
            }

            if (dir < 0 && l.spo2_min > 70)
            {
                l.spo2_min--;
            }
            break;

        /* TEMP MAX */
        case 3:
            if (dir > 0 && l.temp_max_x10 < 450)
            {
                l.temp_max_x10++;
            }

            if (dir < 0 && l.temp_max_x10 > 300)
            {
                l.temp_max_x10--;
            }
            break;

        /* USER ID */
        case 4:
        {
            uint8_t user = SD_Logger_GetUser();

            if (dir > 0 && user < 10U)
            {
                user++;
            }

            if (dir < 0 && user > 0U)
            {
                user--;
            }

            SD_Logger_SetUser(user);
            return;
        }

        default:
            return;
    }

    HealthLimits_Set(l);
}

/* Task xu ly nut nhan */
void Health_Buttons_Task(void *argument)
{
    uint8_t prev = 0;
    uint8_t stable = 0;
    uint8_t count = 0;
    uint8_t raw;

    (void)argument;

    buttons_init();

    for (;;)
    {
        raw = 0;

        /* Doc trang thai cac nut */
        if (GPIO_ReadInputDataBit(
                GPIOA, BTN_SET) == Bit_RESET)
        {
            raw |= 1U;
        }

        if (GPIO_ReadInputDataBit(
                GPIOA, BTN_UP) == Bit_RESET)
        {
            raw |= 2U;
        }

        if (GPIO_ReadInputDataBit(
                GPIOA, BTN_DN) == Bit_RESET)
        {
            raw |= 4U;
        }

        /* Chong doi nut nhan */
        if (raw == prev)
        {
            if (count < 3U)
            {
                count++;
            }
        }
        else
        {
            prev = raw;
            count = 0;
        }

        /* Xu ly mot lan khi nut duoc nhan */
        if (count == 3U && raw != stable)
        {
            uint8_t rising =
                raw & (uint8_t)~stable;

            stable = raw;

            /* SET: chuyen che do 0 -> 4 */
            if (rising & 1U)
            {
                g_mode =
                    (uint8_t)((g_mode + 1U) % 5U);
            }
            /* UP: tang gia tri */
            else if (rising & 2U)
            {
                adjust(+1);
            }
            /* DOWN: giam gia tri */
            else if (rising & 4U)
            {
                adjust(-1);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* Khoi tao LED va Buzzer */
static void warning_init(void)
{
    GPIO_InitTypeDef g;

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOB,
        ENABLE
    );

    GPIO_StructInit(&g);

    g.GPIO_Pin = WARN_LED | WARN_BUZZ;
    g.GPIO_Mode = GPIO_Mode_Out_PP;
    g.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOB, &g);

    GPIO_ResetBits(
        GPIOB,
        WARN_LED | WARN_BUZZ
    );
}

/* Task canh bao */
void Health_Warning_Task(void *argument)
{
    uint8_t on = 0;

    (void)argument;

    warning_init();

    for (;;)
    {
        HealthReading r = HealthData_Get();
        HealthLimits l = HealthLimits_Get();

        /*
         * Chi canh bao theo BPM va nhiet do.
         * Khong su dung SpO2 DEMO de canh bao.
         */
        uint8_t alert =
            (r.bpm_valid &&
             r.bpm > l.bpm_max) ||
            (r.temp_valid &&
             r.temp_x10 > l.temp_max_x10);

        if (alert)
        {
            on = !on;

            if (on)
            {
                GPIO_SetBits(
                    GPIOB,
                    WARN_LED | WARN_BUZZ
                );
            }
            else
            {
                GPIO_ResetBits(
                    GPIOB,
                    WARN_LED | WARN_BUZZ
                );
            }
        }
        else
        {
            on = 0;

            GPIO_ResetBits(
                GPIOB,
                WARN_LED | WARN_BUZZ
            );
        }

        vTaskDelay(pdMS_TO_TICKS(250));
    }
}
