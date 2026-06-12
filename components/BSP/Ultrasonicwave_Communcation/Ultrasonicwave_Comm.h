/*
超声波测距-通信底层
*/

#include "main.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

void Ult_TrigGetDistance(TaskHandle_t taskHandler);

uint16_t Ult_GetDistance(void);