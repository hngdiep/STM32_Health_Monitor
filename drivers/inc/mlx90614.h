
#ifndef MLX90614_H
#define MLX90614_H

#include <stdint.h>

int MLX90614_ReadCentiC(uint8_t reg, int32_t *temperature);

#endif
