
#ifndef SENSOR_I2C1_H
#define SENSOR_I2C1_H

#include <stdint.h>

void SensorI2C1_Init(void);
int SensorI2C1_Probe(uint8_t address7);


int SensorI2C1_ReadRegister8(uint8_t address7, uint8_t reg, uint8_t *value);

int SensorI2C1_WriteRegister8(uint8_t address7, uint8_t reg, uint8_t value);

int SensorI2C1_ReadBurst(uint8_t address7, uint8_t reg, uint8_t *buffer, uint8_t length);


#endif
