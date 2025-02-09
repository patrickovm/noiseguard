#ifndef DISPLAY_H
#define DISPLAY_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define DISPLAY_UPDATE_MS 500

void vTaskUpdateDisplay(void *pvParameters);

extern SemaphoreHandle_t displayMutex;
extern uint8_t display_buffer[];

#endif // DISPLAY_H