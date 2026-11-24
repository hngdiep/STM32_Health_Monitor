#include <stddef.h>
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "i2c1.h"
#include "max30102.h"

volatile uint32_t g_max30102_found = 0;
volatile uint32_t g_max30102_part_id = 0xFFFFFFFFU;
volatile uint32_t g_max30102_read_ok = 0;
volatile uint32_t g_max30102_id_ok = 0;
volatile uint32_t g_max30102_write_ok = 0;
volatile uint32_t g_max30102_id_attempts = 0;
volatile uint32_t g_max30102_id_zero_count = 0;
volatile uint32_t g_max30102_id_i2c_errors = 0;
volatile uint32_t g_max30102_led1_pa = 0xFFFFFFFFU;
volatile uint32_t g_max30102_config_ok = 0;
volatile uint32_t g_max30102_mode_config = 0xFFFFFFFFU;
volatile uint32_t g_max30102_spo2_config = 0xFFFFFFFFU;
volatile uint32_t g_max30102_fifo_wr_ptr = 0;
volatile uint32_t g_max30102_fifo_rd_ptr = 0;
volatile uint32_t g_max30102_fifo_move_count = 0;
volatile uint32_t g_max30102_red_raw = 0;
volatile uint32_t g_max30102_ir_raw = 0;
volatile uint32_t g_max30102_fifo_read_ok = 0;
volatile uint32_t g_max30102_sample_count = 0;
volatile uint32_t g_max30102_fifo_errors = 0;
volatile uint32_t g_max30102_ovf_last = 0;
volatile uint32_t g_max30102_ovf_max = 0;
volatile uint32_t g_max30102_ovf_nonzero_polls = 0;
volatile uint32_t g_max30102_ovf_read_errors = 0;

static uint8_t s_previous_wr = 0U;
static uint8_t s_first_read = 1U;

int MAX30102_Probe(void)
{
    g_max30102_found = (uint32_t)SensorI2C1_Probe(0x57);
    return (int)g_max30102_found;
}

void MAX30102_Init(void)
{
    uint8_t part_id = 0U;
    uint8_t value = 0U;
    uint8_t mode = 0U;
    uint8_t spo2_cfg = 0U;
    int ok = 0;

    s_previous_wr = 0U;
    s_first_read = 1U;

    if (g_max30102_found)
    {
        vTaskDelay(pdMS_TO_TICKS(100));

        for (uint32_t i = 0; i < 10U; ++i)
        {
            part_id = 0xA5U;
            ++g_max30102_id_attempts;

            if (SensorI2C1_ReadRegister8(0x57, 0xFF, &part_id))
            {
                g_max30102_read_ok = 1U;
                g_max30102_part_id = part_id;

                if (part_id == 0x15U)
                {
                    g_max30102_id_ok = 1U;
                    break;
                }

                if (part_id == 0U)
                {
                    ++g_max30102_id_zero_count;
                }
            }
            else
            {
                ++g_max30102_id_i2c_errors;
            }

            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }

    if (g_max30102_id_ok)
    {
        ok = SensorI2C1_WriteRegister8(0x57, 0x09, 0x40);

        if (ok)
        {
            ok = 0;

            for (uint32_t i = 0; i < 20; ++i)
            {
                vTaskDelay(pdMS_TO_TICKS(2));

                if (SensorI2C1_ReadRegister8(0x57, 0x09, &value) &&
                    (value & 0x40U) == 0)
                {
                    ok = 1;
                    break;
                }
            }
        }

        if (ok)
        {
            ok =
                SensorI2C1_WriteRegister8(0x57, 0x08, 0x10) &&
                SensorI2C1_WriteRegister8(0x57, 0x04, 0x00) &&
                SensorI2C1_WriteRegister8(0x57, 0x05, 0x00) &&
                SensorI2C1_WriteRegister8(0x57, 0x06, 0x00) &&
                SensorI2C1_WriteRegister8(0x57, 0x0A, 0x27) &&
                SensorI2C1_WriteRegister8(0x57, 0x0C, 0x24) &&
                SensorI2C1_WriteRegister8(0x57, 0x0D, 0x24) &&
                SensorI2C1_WriteRegister8(0x57, 0x09, 0x03);
        }

        if (ok)
        {
            ok =
                SensorI2C1_ReadRegister8(0x57, 0x09, &mode) &&
                SensorI2C1_ReadRegister8(0x57, 0x0A, &spo2_cfg) &&
                SensorI2C1_ReadRegister8(0x57, 0x0C, &value);

            if (ok)
            {
                g_max30102_mode_config = mode;
                g_max30102_spo2_config = spo2_cfg;
                g_max30102_led1_pa = value;

                ok = (mode == 0x03 &&
                      spo2_cfg == 0x27 &&
                      value == 0x24);
            }
        }

        g_max30102_write_ok = ok;
        g_max30102_config_ok = ok;
    }
}

uint8_t MAX30102_PollAvailable(uint8_t *overflow_out)
{
    uint8_t status1 = 0U;
    uint8_t status_ok = 0U;
    uint8_t ovf = 0U;
    uint8_t wr = 0U;
    uint8_t rd = 0U;
    uint8_t available = 0U;

    if (overflow_out != NULL)
    {
        *overflow_out = 0U;
    }

    if (!g_max30102_config_ok)
    {
        return 0U;
    }

    status_ok = (uint8_t)SensorI2C1_ReadRegister8(0x57, 0x00, &status1);

    if (SensorI2C1_ReadRegister8(0x57, 0x05, &ovf))
    {
        uint32_t overflow = (uint32_t)(ovf & 0x1FU);

        g_max30102_ovf_last = overflow;

        if (overflow > g_max30102_ovf_max)
        {
            g_max30102_ovf_max = overflow;
        }

        if (overflow > 0U)
        {
            ++g_max30102_ovf_nonzero_polls;
        }

        if (overflow_out != NULL)
        {
            *overflow_out = (uint8_t)overflow;
        }
    }
    else
    {
        ++g_max30102_ovf_read_errors;
    }

    if (SensorI2C1_ReadRegister8(0x57, 0x04, &wr) &&
        SensorI2C1_ReadRegister8(0x57, 0x06, &rd))
    {
        wr &= 0x1FU;
        rd &= 0x1FU;

        g_max30102_fifo_wr_ptr = wr;
        g_max30102_fifo_rd_ptr = rd;

        if (!s_first_read && wr != s_previous_wr)
        {
            ++g_max30102_fifo_move_count;
        }

        s_previous_wr = wr;
        s_first_read = 0U;

        available = (uint8_t)((wr - rd) & 0x1FU);

        if ((available == 0U) &&
            (((ovf & 0x1FU) != 0U) ||
             ((status_ok != 0U) && ((status1 & 0x80U) != 0U))))
        {
            available = 32U;
        }
    }
    else
    {
        g_max30102_fifo_read_ok = 0U;
        ++g_max30102_fifo_errors;
    }

    return available;
}

int MAX30102_ReadSample(uint32_t *red_raw, uint32_t *ir_raw)
{
    uint8_t fifo[6];

    if (red_raw == NULL || ir_raw == NULL)
    {
        return 0;
    }

    if (!SensorI2C1_ReadBurst(0x57, 0x07, fifo, 6))
    {
        g_max30102_fifo_read_ok = 0U;
        ++g_max30102_fifo_errors;
        return 0;
    }

    g_max30102_red_raw =
        ((((uint32_t)fifo[0] << 16) |
          ((uint32_t)fifo[1] << 8) |
          (uint32_t)fifo[2]) & 0x3FFFFU);

    g_max30102_ir_raw =
        ((((uint32_t)fifo[3] << 16) |
          ((uint32_t)fifo[4] << 8) |
          (uint32_t)fifo[5]) & 0x3FFFFU);

    *red_raw = g_max30102_red_raw;
    *ir_raw = g_max30102_ir_raw;

    g_max30102_fifo_read_ok = 1U;
    ++g_max30102_sample_count;
    return 1;
}
