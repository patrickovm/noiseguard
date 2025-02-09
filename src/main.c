#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306_i2c.h"
#include "ssd1306.h"
#include "ds18b20.h"

#include <string.h>

#define LED_RED_PIN 13
#define LED_GREEN_PIN 11
#define LED_BLUE_PIN 12
#define BUZZER_PIN 21
#define BUTTON_A_PIN 5
#define DS18B20_PIN 4

static DS18B20 temp_sensor;
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 15

// Temperature thresholds (in Celsius)
static float temp_threshold = 25.0f;
static uint8_t display_buffer[ssd1306_buffer_length];

TaskHandle_t tempTaskHandle;
TaskHandle_t displayTaskHandle;
TaskHandle_t alertTaskHandle;

void vTempTask(void *pvParameters);
void vDisplayTask(void *pvParameters);
void vAlertTask(void *pvParameters);
void init_hardware(void);

QueueHandle_t xTempQueue;

int main()
{
    stdio_init_all();
    init_hardware();

    xTempQueue = xQueueCreate(1, sizeof(float));

    xTaskCreate(vTempTask, "TempTask", configMINIMAL_STACK_SIZE, NULL, 2, &tempTaskHandle);
    xTaskCreate(vDisplayTask, "DisplayTask", configMINIMAL_STACK_SIZE, NULL, 1, &displayTaskHandle);
    xTaskCreate(vAlertTask, "AlertTask", configMINIMAL_STACK_SIZE, NULL, 1, &alertTaskHandle);

    vTaskStartScheduler();

    while (1)
    {
        // Should never get here
    }
}

void init_hardware(void)
{
    if (!ds18b20_init(&temp_sensor, pio0, DS18B20_PIN))
    {
        printf("Failed to initialize DS18B20\n");
    }

    i2c_init(i2c1, 400000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    ssd1306_init();

    gpio_init(LED_RED_PIN);
    gpio_init(LED_GREEN_PIN);
    gpio_init(LED_BLUE_PIN);
    gpio_init(BUZZER_PIN);
    gpio_init(BUTTON_A_PIN);

    gpio_set_dir(LED_RED_PIN, GPIO_OUT);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    gpio_set_dir(BUTTON_A_PIN, GPIO_IN);

    gpio_pull_up(BUTTON_A_PIN);
}

void vTempTask(void *pvParameters)
{
    float temperature;
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        temperature = ds18b20_read_temp(&temp_sensor);

        xQueueOverwrite(xTempQueue, &temperature);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

void vDisplayTask(void *pvParameters)
{
    float temperature;
    char str[32];
    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1};
    calculate_render_area_buffer_length(&area);

    while (1)
    {
        if (xQueuePeek(xTempQueue, &temperature, 0) == pdTRUE)
        {
            memset(display_buffer, 0, sizeof(display_buffer));

            snprintf(str, sizeof(str), "Temp: %.1fC", temperature);
            ssd1306_draw_string(display_buffer, 0, 0, str);

            snprintf(str, sizeof(str), "Limit: %.1fC", temp_threshold);
            ssd1306_draw_string(display_buffer, 0, 16, str);

            render_on_display(display_buffer, &area);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vAlertTask(void *pvParameters)
{
    float temperature;
    bool alert_state = false;

    while (1)
    {
        if (xQueuePeek(xTempQueue, &temperature, 0) == pdTRUE)
        {
            if (temperature > temp_threshold)
            {
                // Red LED and buzzer for high temperature
                gpio_put(LED_RED_PIN, 1);
                gpio_put(LED_GREEN_PIN, 0);
                gpio_put(LED_BLUE_PIN, 0);

                if (!alert_state)
                {
                    gpio_put(BUZZER_PIN, 1);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    gpio_put(BUZZER_PIN, 0);
                    alert_state = true;
                }
            }
            else
            {
                // Green LED for normal temperature
                gpio_put(LED_RED_PIN, 0);
                gpio_put(LED_GREEN_PIN, 1);
                gpio_put(LED_BLUE_PIN, 0);
                alert_state = false;
            }
        }

        // Check button for threshold change
        if (!gpio_get(BUTTON_A_PIN))
        {
            temp_threshold += 5.0f;
            if (temp_threshold > 40.0f)
            {
                temp_threshold = 20.0f;
            }
            vTaskDelay(pdMS_TO_TICKS(300)); // Debounce
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
