#include "input.h"
#include "peripherals.h"
#include "noise_monitor.h"

void vTaskHandleInput(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const int joystick_center = 2048;
    const int deadzone = 500;

    while (1)
    {
        if (!gpio_get(BTN_A))
        {
            warning_threshold -= 100;
            danger_threshold -= 100;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (!gpio_get(BTN_B))
        {
            warning_threshold += 100;
            danger_threshold += 100;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        int joystick_value = read_joystick_x();
        if (abs(joystick_value - joystick_center) > deadzone)
        {
            if (joystick_value > joystick_center)
            {
                threshold_gap = threshold_gap < MAX_GAP ? threshold_gap + 50 : MAX_GAP;
            }
            else
            {
                threshold_gap = threshold_gap > MIN_GAP ? threshold_gap - 50 : MIN_GAP;
            }
            danger_threshold = warning_threshold + threshold_gap;
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}