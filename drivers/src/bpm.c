
#include "bpm.h"

volatile uint32_t g_bpm_x100 = 0;
volatile uint32_t g_bpm_valid = 0;
volatile uint32_t g_bpm_detected_peaks = 0;
volatile uint32_t g_bpm_last_interval_samples = 0;

static int32_t s_baseline = 0;
static int32_t s_filtered = 0;
static int32_t s_envelope = 0;
static int32_t s_prev1 = 0;
static int32_t s_prev2 = 0;

static uint32_t s_sample_index = 0;
static uint32_t s_last_peak_index = 0;

static uint16_t s_intervals[3] = {0};
static uint8_t s_interval_count = 0;
static uint8_t s_interval_position = 0;
static uint8_t s_started = 0;

void BPM_Init(void)
{
    g_bpm_x100 = 0;
    g_bpm_valid = 0;
    g_bpm_detected_peaks = 0;
    g_bpm_last_interval_samples = 0;

    s_baseline = 0;
    s_filtered = 0;
    s_envelope = 0;
    s_prev1 = 0;
    s_prev2 = 0;

    s_sample_index = 0;
    s_last_peak_index = 0;
    s_interval_count = 0;
    s_interval_position = 0;
    s_started = 0;
}

void BPM_ProcessIR(uint32_t ir_raw)
{
    if (ir_raw < 10000U)
    {
        BPM_Init();
        return;
    }

    if (!s_started)
    {
        s_baseline = (int32_t)ir_raw;
        s_started = 1;
        return;
    }

    ++s_sample_index;

    s_baseline += ((int32_t)ir_raw - s_baseline) / 64;

    int32_t ac = s_baseline - (int32_t)ir_raw;

    s_filtered += (ac - s_filtered) / 4;

    int32_t magnitude = s_filtered >= 0 ? s_filtered : -s_filtered;

    s_envelope += (magnitude - s_envelope) / 32;

    int32_t threshold = s_envelope / 2;

    if (threshold < 120)
        threshold = 120;

    uint8_t peak =
        s_prev1 > s_prev2 &&
        s_prev1 >= s_filtered &&
        s_prev1 > threshold;

    s_prev2 = s_prev1;
    s_prev1 = s_filtered;

    if (s_sample_index < 100U)
        return;

    uint32_t current_index = s_sample_index - 1U;

    if (s_last_peak_index != 0U &&
        current_index - s_last_peak_index > 200U)
    {
        g_bpm_x100 = 0;
        g_bpm_valid = 0;
        s_interval_count = 0;
        s_interval_position = 0;
        s_last_peak_index = 0;
    }

    if (!peak)
        return;

    if (s_last_peak_index != 0U)
    {
        uint32_t interval = current_index - s_last_peak_index;

        if (interval < 40U)
            return;

        if (interval <= 150U)
        {
            s_intervals[s_interval_position] = (uint16_t)interval;
            s_interval_position = (uint8_t)((s_interval_position + 1U) % 3U);

            if (s_interval_count < 3U)
                ++s_interval_count;

            g_bpm_last_interval_samples = interval;

            if (s_interval_count == 3U)
            {
                uint16_t a = s_intervals[0];
                uint16_t b = s_intervals[1];
                uint16_t c = s_intervals[2];
                uint16_t temp;

                if (a > b)
                {
                    temp = a;
                    a = b;
                    b = temp;
                }

                if (b > c)
                {
                    temp = b;
                    b = c;
                    c = temp;
                }

                if (a > b)
                {
                    temp = a;
                    a = b;
                    b = temp;
                }

                g_bpm_x100 = (600000U + b / 2U) / b;
                g_bpm_valid = 1U;
            }
        }
        else
        {
            s_interval_count = 0;
            s_interval_position = 0;
            g_bpm_valid = 0;
            g_bpm_x100 = 0;
        }
    }

    s_last_peak_index = current_index;
    ++g_bpm_detected_peaks;
}
