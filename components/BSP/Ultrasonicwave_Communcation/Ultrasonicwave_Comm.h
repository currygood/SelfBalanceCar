/*
超声波测距-通信底层
*/

#include "main.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

void Ult_TrigGetDistance(EventGroupHandle_t eventgoup);

uint16_t Ult_GetDistance(void);