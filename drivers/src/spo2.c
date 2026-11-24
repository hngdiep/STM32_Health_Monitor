
#include "spo2.h"
#include <math.h>

#define SPO2_WINDOW_SIZE 400U
#define SPO2_SETTLE_SAMPLES 30U
#define SPO2_WARMUP_SAMPLES 100U

volatile uint32_t g_spo2_ratio_x10000 = 0;
volatile uint32_t g_spo2_demo_x100 = 0;
volatile uint32_t g_spo2_valid = 0;
volatile uint32_t g_spo2_window_count = 0;
volatile uint32_t g_spo2_correlation_x1000 = 0;

static uint32_t s_settle = 0;
static uint32_t s_warmup = 0;
static uint32_t s_count = 0;

static float s_red_dc = 0.0f;
static float s_ir_dc = 0.0f;
static float s_red_fast = 0.0f;
static float s_ir_fast = 0.0f;

static float s_red_energy = 0.0f;
static float s_ir_energy = 0.0f;
static float s_cross_energy = 0.0f;

void SpO2_Init(void)
{
    g_spo2_ratio_x10000 = 0;
    g_spo2_demo_x100 = 0;
    g_spo2_valid = 0;
    g_spo2_window_count = 0;
    g_spo2_correlation_x1000 = 0;

    s_settle = 0;
    s_warmup = 0;
    s_count = 0;

    s_red_dc = 0.0f;
    s_ir_dc = 0.0f;
    s_red_fast = 0.0f;
    s_ir_fast = 0.0f;

    s_red_energy = 0.0f;
    s_ir_energy = 0.0f;
    s_cross_energy = 0.0f;
}

void SpO2_ProcessSample(uint32_t red_raw, uint32_t ir_raw)
{
    if (red_raw < 10000U || ir_raw < 10000U ||
        red_raw > 0x3FFFFU || ir_raw > 0x3FFFFU)
    {
        SpO2_Init();
        return;
    }

    float red = (float)red_raw;
    float ir = (float)ir_raw;

    if (s_settle < SPO2_SETTLE_SAMPLES)
    {
        ++s_settle;

        if (s_settle == SPO2_SETTLE_SAMPLES)
        {
            s_red_dc = red;
            s_ir_dc = ir;
            s_red_fast = red;
            s_ir_fast = ir;
        }

        return;
    }

    s_red_dc += (red - s_red_dc) / 128.0f;
    s_ir_dc += (ir - s_ir_dc) / 128.0f;

    s_red_fast += (red - s_red_fast) / 4.0f;
    s_ir_fast += (ir - s_ir_fast) / 4.0f;

    if (s_warmup < SPO2_WARMUP_SAMPLES)
    {
        ++s_warmup;
        return;
    }

    float red_ac = s_red_fast - s_red_dc;
    float ir_ac = s_ir_fast - s_ir_dc;

    s_red_energy += red_ac * red_ac;
    s_ir_energy += ir_ac * ir_ac;
    s_cross_energy += red_ac * ir_ac;

    ++s_count;

    if (s_count < SPO2_WINDOW_SIZE)
        return;

    float red_rms = sqrtf(s_red_energy / (float)SPO2_WINDOW_SIZE);
    float ir_rms = sqrtf(s_ir_energy / (float)SPO2_WINDOW_SIZE);

    g_spo2_valid = 0;
    g_spo2_demo_x100 = 0;
    g_spo2_ratio_x10000 = 0;
    g_spo2_correlation_x1000 = 0;
    ++g_spo2_window_count;

    if (red_rms > 15.0f && ir_rms > 15.0f &&
        s_red_dc > 10000.0f && s_ir_dc > 10000.0f)
    {
        float ratio = (red_rms / s_red_dc) /
                      (ir_rms / s_ir_dc);

        float correlation = s_cross_energy /
                            sqrtf(s_red_energy * s_ir_energy);

        if (ratio > 0.0f && ratio < 10.0f)
        {
            g_spo2_ratio_x10000 =
                (uint32_t)(ratio * 10000.0f + 0.5f);
        }

        if (correlation > 0.0f && correlation <= 1.0f)
        {
            g_spo2_correlation_x1000 =
                (uint32_t)(correlation * 1000.0f + 0.5f);
        }

        if (ratio >= 0.2f && ratio <= 2.0f &&
            correlation >= 0.8f)
        {
            float estimate = 110.0f - 25.0f * ratio;

            if (estimate > 100.0f)
                estimate = 100.0f;

            if (estimate < 0.0f)
                estimate = 0.0f;

            g_spo2_demo_x100 =
                (uint32_t)(estimate * 100.0f + 0.5f);

            g_spo2_valid = 1;
        }
    }

    s_count = 0;
    s_red_energy = 0.0f;
    s_ir_energy = 0.0f;
    s_cross_energy = 0.0f;
}
