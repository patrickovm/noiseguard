#include <stdio.h>
#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "task.h"

#include "peripherals.h"
#include "noise_monitor.h"
#include "display.h"
#include "input.h"

int main()
{
    stdio_init_all();
    init_peripherals();

    displayMutex = xSemaphoreCreateMutex();

    xTaskCreate(vTaskMonitorNoise, "NoiseMonitorTask", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(vTaskUpdateDisplay, "DisplayUpdateTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vTaskHandleInput, "InputHandlerTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1)
    {
        // Should never get here
    };

    return 0;
}