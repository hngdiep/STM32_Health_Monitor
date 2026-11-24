
#include "sd_rtc.h"
#include "stm32f10x.h"
#include "ff.h"

#define RTC_BACKUP_MAGIC 0xA53C
#define RTC_POLL_LIMIT 2000000
#define LSI_PRESCALER 39999  /* Nominal LSI 40 kHz -> ~1 Hz. */
#define LSE_PRESCALER 32767  /* LSE 32.768 kHz -> 1 Hz. */

static bool rtc_ready;

/* RTC->CRL is a 16-bit register. */
static bool wait_for_bit16(volatile uint16_t *reg, uint16_t mask, bool set)
{
    for (int n = 0; n < RTC_POLL_LIMIT; n++) {
        if (((*reg & mask) != 0) == set) return true;
    }
    return false;
}

/* RCC->CSR is a 32-bit register. */
static bool wait_for_bit32(volatile uint32_t *reg, uint32_t mask, bool set)
{
    for (int n = 0; n < RTC_POLL_LIMIT; n++) {
        if (((*reg & mask) != 0) == set) return true;
    }
    return false;
}

static bool rtc_wait_sync(void)
{
    if (!wait_for_bit16(&RTC->CRL, RTC_CRL_RTOFF, true)) return false;
    RTC->CRL &= ~RTC_CRL_RSF;
    return wait_for_bit16(&RTC->CRL, RTC_CRL_RSF, true);
}

static bool rtc_start_write(void)
{
    if (!wait_for_bit16(&RTC->CRL, RTC_CRL_RTOFF, true)) return false;
    RTC->CRL |= RTC_CRL_CNF;
    return true;
}

static bool rtc_end_write(void)
{
    RTC->CRL &= ~RTC_CRL_CNF;
    return wait_for_bit16(&RTC->CRL, RTC_CRL_RTOFF, true);
}

static bool leap(int year)
{
    return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
}

static uint8_t days_in_month(int year, int month)
{
    static const uint8_t days[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    if (month < 1 || month > 12) return 0;
    if (month == 2 && leap(year)) return 29;
    return days[month - 1];
}

static bool datetime_to_seconds(const SD_RTC_DateTime *dt, uint32_t *seconds)
{
    if (dt == 0 || seconds == 0 ||
        dt->year < 2000 || dt->year > 2099 ||
        dt->month < 1 || dt->month > 12 ||
        dt->day < 1 || dt->day > days_in_month(dt->year, dt->month) ||
        dt->hour > 23 || dt->minute > 59 || dt->second > 59) {
        return false;
    }

    uint32_t days = 0;

    for (int year = 2000; year < dt->year; year++) {
        days += leap(year) ? 366 : 365;
    }

    for (int month = 1; month < dt->month; month++) {
        days += days_in_month(dt->year, month);
    }

    days += dt->day - 1;

    *seconds = days * 86400 +
               dt->hour * 3600 +
               dt->minute * 60 +
               dt->second;

    return true;
}

bool SD_RTC_Init(void)
{
    rtc_ready = false;

    RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR->CR |= PWR_CR_DBP;

    /*
     * LSI does not require an external 32.768 kHz crystal.
     * Preserve an existing LSE RTC configuration when present.
     */
    RCC->CSR |= RCC_CSR_LSION;

    if (!wait_for_bit32(&RCC->CSR, RCC_CSR_LSIRDY, true)) {
        return false;
    }

    if ((RCC->BDCR & RCC_BDCR_RTCSEL) == 0) {
        RCC->BDCR = (RCC->BDCR & ~RCC_BDCR_RTCSEL) |
                    RCC_BDCR_RTCSEL_1;
    }

    RCC->BDCR |= RCC_BDCR_RTCEN;

    if (!rtc_wait_sync()) {
        return false;
    }

    if (BKP->DR1 != RTC_BACKUP_MAGIC) {
        uint32_t clock_source = RCC->BDCR & RCC_BDCR_RTCSEL;

        uint32_t prescaler =
            (clock_source == RCC_BDCR_RTCSEL_0) ?
            LSE_PRESCALER : LSI_PRESCALER;

        if (!rtc_start_write()) {
            return false;
        }

        RTC->PRLH = (prescaler >> 16) & 0xFFFF;
        RTC->PRLL = prescaler & 0xFFFF;

        RTC->CNTH = 0;
        RTC->CNTL = 0;
        RTC->CRH = 0;

        if (!rtc_end_write()) {
            return false;
        }

        /* Date becomes valid after SD_RTC_SetDateTime(). */
    }

    rtc_ready = true;
    return true;
}

bool SD_RTC_IsSet(void)
{
    return rtc_ready && BKP->DR1 == RTC_BACKUP_MAGIC;
}

bool SD_RTC_SetDateTime(const SD_RTC_DateTime *dt)
{
    uint32_t timestamp;

    if (!rtc_ready || !datetime_to_seconds(dt, &timestamp)) {
        return false;
    }

    if (!rtc_start_write()) {
        return false;
    }

    RTC->CNTH = (timestamp >> 16) & 0xFFFF;
    RTC->CNTL = timestamp & 0xFFFF;

    if (!rtc_end_write()) {
        return false;
    }

    BKP->DR1 = RTC_BACKUP_MAGIC;
    return true;
}

uint32_t SD_RTC_Now(void)
{
    if (!SD_RTC_IsSet()) {
        return SD_RTC_INVALID_TIMESTAMP;
    }

    uint32_t high1, high2, low;

    do {
        high1 = RTC->CNTH;
        low = RTC->CNTL;
        high2 = RTC->CNTH;
    } while (high1 != high2);

    return (high1 << 16) | low;
}

bool SD_RTC_ToDateTime(uint32_t timestamp, SD_RTC_DateTime *dt)
{
    if (dt == 0 || timestamp == SD_RTC_INVALID_TIMESTAMP) {
        return false;
    }

    uint32_t day_count = timestamp / 86400;
    uint32_t day_sec = timestamp % 86400;

    int year = 2000;

    while (year <= 2099) {
        uint32_t year_days = leap(year) ? 366 : 365;

        if (day_count < year_days) break;

        day_count -= year_days;
        year++;
    }

    if (year > 2099) {
        return false;
    }

    int month = 1;

    while (month <= 12) {
        uint32_t m_days = days_in_month(year, month);

        if (day_count < m_days) break;

        day_count -= m_days;
        month++;
    }

    dt->year = (uint16_t)year;
    dt->month = (uint8_t)month;
    dt->day = (uint8_t)(day_count + 1);
    dt->hour = (uint8_t)(day_sec / 3600);
    dt->minute = (uint8_t)((day_sec / 60) % 60);
    dt->second = (uint8_t)(day_sec % 60);

    return true;
}

bool SD_RTC_GetDateTime(SD_RTC_DateTime *dt)
{
    return SD_RTC_ToDateTime(SD_RTC_Now(), dt);
}

/* FatFS uses DOS timestamps, not Unix timestamps. */
DWORD get_fattime(void)
{
    SD_RTC_DateTime dt;

    if (!SD_RTC_GetDateTime(&dt)) {
        /* 1980-01-01 */
        return (DWORD)((0 << 25) | (1 << 21) | (1 << 16));
    }

    return (DWORD)(
        ((uint32_t)(dt.year - 1980) << 25) |
        ((uint32_t)dt.month << 21) |
        ((uint32_t)dt.day << 16) |
        ((uint32_t)dt.hour << 11) |
        ((uint32_t)dt.minute << 5) |
        (dt.second / 2)
    );
}
