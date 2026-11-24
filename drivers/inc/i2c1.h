
#ifndef SENSOR_I2C1_H
#define SENSOR_I2C1_H

#include <stdint.h>

void SensorI2C1_Init(void);
int SensorI2C1_Probe(uint8_t address7);

#endif
