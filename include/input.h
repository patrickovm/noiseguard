#ifndef INPUT_H
#define INPUT_H

#include "FreeRTOS.h"
#include "task.h"

void vTaskHandleInput(void *pvParameters);

#endif // INPUT_H