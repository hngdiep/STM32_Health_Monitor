
#include "mlx90614.h"
#include "i2c1.h"

static uint8_t MLX90614_CRC8(const uint8_t *data, uint8_t length)
{
    uint8_t crc = 0;

    for (uint8_t i = 0; i < length; ++i)
    {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8U; ++bit)
        {
            crc = (crc & 0x80U)
                ? (uint8_t)((crc << 1) ^ 0x07U)
                : (uint8_t)(crc << 1);
        }
    }

    return crc;
}

int MLX90614_ReadCentiC(uint8_t reg, int32_t *temperature)
{
    uint8_t data[3];

    if (temperature == 0 || (reg != 0x06U && reg != 0x07U))
        return 0;

    if (!SensorI2C1_ReadBurst(0x5A, reg, data, 3))
        return 0;

    uint8_t frame[5] = {
        0xB4U,
        reg,
        0xB5U,
        data[0],
        data[1]
    };

    if (MLX90614_CRC8(frame, 5) != data[2])
        return 0;

    uint16_t raw = (uint16_t)data[0] |
                   ((uint16_t)data[1] << 8);

    if (raw & 0x8000U)
        return 0;

    *temperature = (int32_t)raw * 2 - 27315;

    return 1;
}
