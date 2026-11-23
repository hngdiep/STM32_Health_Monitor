#ifndef HEALTH_CONTROLS_H
#define HEALTH_CONTROLS_H
#include <stdint.h>
/* SET cycles: 0=monitor, 1=upper BPM, 2=lower SpO2, 3=upper temperature */
uint8_t HealthControls_GetMode(void);
void Health_Buttons_Task(void *argument);
void Health_Warning_Task(void *argument);
#endif
