#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "ssd1306.h"

#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12
#define BTN_A 5
#define BTN_B 6
#define MIC_IN 28
#define JOYSTICK_X 27
#define JOYSTICK_Y 26
#define JOYSTICK_SW 22
#define I2C_SDA 14
#define I2C_SCL 15

#define NOISE_THRESHOLD_WARNING 2000
#define NOISE_THRESHOLD_DANGER 3000
#define SAMPLE_RATE_MS 100
#define DISPLAY_UPDATE_MS 500

static SemaphoreHandle_t displayMutex;

static uint8_t display_buffer[ssd1306_buffer_length];
static int noise_level = 0;
static int warning_threshold = NOISE_THRESHOLD_WARNING;
static int danger_threshold = NOISE_THRESHOLD_DANGER;

void vTaskMonitorNoise(void *pvParameters);
void vTaskUpdateDisplay(void *pvParameters);
void vTaskHandleInput(void *pvParameters);

void update_led_status(int level);
void init_peripherals(void);

void init_peripherals(void)
{
    gpio_init(LED_RED);
    gpio_init(LED_GREEN);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    gpio_init(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_pull_up(BTN_B);

    adc_init();
    adc_gpio_init(MIC_IN);
    adc_select_input(2);

    i2c_init(i2c1, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();
}

void vTaskMonitorNoise(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        uint16_t raw = adc_read();
        noise_level = raw;
        update_led_status(raw);

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SAMPLE_RATE_MS));
    }
}

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
            // Clear buffer
            memset(display_buffer, 0, sizeof(display_buffer));

            ssd1306_draw_string(display_buffer, 0, 0, "Noise Guard");

            // Draw current level
            char levelStr[32];
            snprintf(levelStr, sizeof(levelStr), "Level: %d", noise_level);
            ssd1306_draw_string(display_buffer, 0, 16, levelStr);

            // Draw thresholds
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

void update_led_status(int level)
{
    if (level < warning_threshold)
    {
        // Green - OK
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 0);
    }
    else if (level < danger_threshold)
    {
        // Yellow - Warning
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 0);
    }
    else
    {
        // Red - Danger
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 0);
    }
}

void vTaskHandleInput(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        if (!gpio_get(BTN_A))
        {
            warning_threshold -= 100;
            danger_threshold -= 100;
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
        }

        if (!gpio_get(BTN_B))
        {
            warning_threshold += 100;
            danger_threshold += 100;
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
        }

        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}

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
    };

    return 0;
}