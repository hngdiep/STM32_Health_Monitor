
#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sd_rtc.h"
#include "sd_task.h"

/* 1: Test du lieu gia lap, 0: Cam bien that */
#define SD_ENABLE_DEMO_SENSOR 1

/* Thoi gian RTC khoi tao lan dau */
#define SD_DEMO_YEAR    2026
#define SD_DEMO_MONTH   11
#define SD_DEMO_DAY     24
#define SD_DEMO_HOUR    9
#define SD_DEMO_MINUTE  0
#define SD_DEMO_SECOND  0

/* Cau hinh Clock 72MHz */
static void SystemClock_Config(void)
{
    RCC->CR |= RCC_CR_HSEON;

    int timeout = 2000000;

    while ((RCC->CR & RCC_CR_HSERDY) == 0 &&
           --timeout > 0) {
    }

    if ((RCC->CR & RCC_CR_HSERDY) == 0) {
        SystemCoreClockUpdate();
        return;
    }

    FLASH->ACR = FLASH_ACR_PRFTBE |
                 FLASH_ACR_LATENCY_2;

    RCC->CFGR =
        (RCC->CFGR &
        ~(RCC_CFGR_HPRE |
          RCC_CFGR_PPRE1 |
          RCC_CFGR_PPRE2 |
          RCC_CFGR_PLLSRC |
          RCC_CFGR_PLLMULL |
          RCC_CFGR_PLLXTPRE)) |
        RCC_CFGR_PPRE1_DIV2 |
        RCC_CFGR_PLLSRC |
        RCC_CFGR_PLLMULL9;

    RCC->CR |= RCC_CR_PLLON;

    timeout = 2000000;

    while ((RCC->CR & RCC_CR_PLLRDY) == 0 &&
           --timeout > 0) {
    }

    if ((RCC->CR & RCC_CR_PLLRDY) == 0) {
        SystemCoreClockUpdate();
        return;
    }

    RCC->CFGR =
        (RCC->CFGR & ~RCC_CFGR_SW) |
        RCC_CFGR_SW_PLL;

    timeout = 2000000;

    while ((RCC->CFGR & RCC_CFGR_SWS) !=
           RCC_CFGR_SWS_PLL &&
           --timeout > 0) {
    }

    SystemCoreClockUpdate();
}

/* LED PC13 onboard */
static void LED_PC13_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC->CRH &= ~(0xF << 20);

    /* Output push-pull, 2MHz */
    GPIOC->CRH |= (0x2 << 20);

    /* LED sang khi PC13 = 0 */
    GPIOC->BRR = (1 << 13);
}

/* Sensor_Task mo phong 3 nguoi dung */
#if SD_ENABLE_DEMO_SENSOR

static void Sensor_Task(void *argument)
{
    (void)argument;

    TickType_t previous = xTaskGetTickCount();

    uint32_t number = 0;

    for (;;) {

        /* Moi 10 mau chuyen sang User tiep theo */
        uint8_t selected_user_id =
            (uint8_t)(1 + (number / 10) % 3);

        SD_LogSample sample = {
            .bpm =
                (uint16_t)(72 + number % 5),

            .spo2 =
                (uint8_t)(97 + number % 3),

            .temperature_centi_c =
                (int16_t)(3650 + number % 11),

            .signal = SIGNAL_VALID,

            .user_id = selected_user_id
        };

        /* Mo phong tin hieu yeu */
        if (number % 20 == 10) {
            sample.signal = SIGNAL_WEAK;
        }

        /* Mo phong mat ngon tay */
        else if (number % 20 == 11) {
            sample.signal = SIGNAL_NO_FINGER;
        }

        /* Gui du lieu vao FreeRTOS Queue */
        (void)SD_Logging_Publish(&sample);

        number++;

        /* Chu ky 1000ms */
        vTaskDelayUntil(
            &previous,
            pdMS_TO_TICKS(1000)
        );
    }
}

#endif

/* FreeRTOS Heap Error */
void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();

    for (;;) {
    }
}

/* FreeRTOS Stack Overflow Error */
void vApplicationStackOverflowHook(
    TaskHandle_t task,
    char *name)
{
    (void)task;
    (void)name;

    taskDISABLE_INTERRUPTS();

    for (;;) {
    }
}

/* ================= MAIN ================= */

int main(void)
{
    /* Khoi tao Clock */
    SystemClock_Config();

    /* Khoi tao LED PC13 */
    LED_PC13_Init();

    /* Khoi tao RTC */
    bool rtc_ok = SD_RTC_Init();

    if (rtc_ok && !SD_RTC_IsSet()) {

        SD_RTC_DateTime time = {
            SD_DEMO_YEAR,
            SD_DEMO_MONTH,
            SD_DEMO_DAY,
            SD_DEMO_HOUR,
            SD_DEMO_MINUTE,
            SD_DEMO_SECOND
        };

        (void)SD_RTC_SetDateTime(&time);
    }

    /* Khoi tao Logging_Task va Queue */
    if (!SD_Logging_Init()) {
        for (;;) {
        }
    }

#if SD_ENABLE_DEMO_SENSOR

    /* Khoi tao Sensor_Task mo phong */
    if (xTaskCreate(
            Sensor_Task,
            "Sensor_Task",
            192,
            0,
            tskIDLE_PRIORITY + 3,
            0) != pdPASS) {

        for (;;) {
        }
    }

#endif

    /* Khoi dong FreeRTOS */
    vTaskStartScheduler();

    for (;;) {
    }
}
