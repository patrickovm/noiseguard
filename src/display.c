#include "display.h"
#include "ssd1306.h"
#include "noise_monitor.h"

SemaphoreHandle_t displayMutex;
uint8_t display_buffer[ssd1306_buffer_length];

void vTaskUpdateDisplay(void *pvParameters)
{
    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1};
    calculate_render_area_buffer_length(&area);

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            memset(display_buffer, 0, sizeof(display_buffer));

            ssd1306_draw_string(display_buffer, 0, 0, "Noise Guard");

            char levelStr[32];
            snprintf(levelStr, sizeof(levelStr), "Level: %d", noise_level);
            ssd1306_draw_string(display_buffer, 0, 16, levelStr);

            snprintf(levelStr, sizeof(levelStr), "Warn: %d", warning_threshold);
            ssd1306_draw_string(display_buffer, 0, 32, levelStr);
            snprintf(levelStr, sizeof(levelStr), "Dang: %d", danger_threshold);
            ssd1306_draw_string(display_buffer, 0, 48, levelStr);

            render_on_display(display_buffer, &area);

            xSemaphoreGive(displayMutex);
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DISPLAY_UPDATE_MS));
    }
}