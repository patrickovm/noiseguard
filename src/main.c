#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "ssd1306.h"

#define LED_RED     13
#define LED_GREEN   11
#define LED_BLUE    12
#define BTN_A        5
#define BTN_B        6
#define MIC_IN      28
#define JOYSTICK_X  27
#define JOYSTICK_Y  26
#define JOYSTICK_SW 22
#define I2C_SDA     14
#define I2C_SCL     15

// Constants
#define NOISE_THRESHOLD_WARNING 2000
#define NOISE_THRESHOLD_DANGER  3000
#define SAMPLE_RATE_MS 100
#define DISPLAY_UPDATE_MS 500

// Global variables
static SemaphoreHandle_t displayMutex;
static uint8_t displayBuffer[ssd1306_buffer_length];
static int noiseLevel = 0;
static int warningThreshold = NOISE_THRESHOLD_WARNING;
static int dangerThreshold = NOISE_THRESHOLD_DANGER;

// Function prototypes
void vTaskMonitorNoise(void *pvParameters);
void vTaskUpdateDisplay(void *pvParameters);
void vTaskHandleInput(void *pvParameters);
void updateLEDStatus(int level);
void initializeHardware(void);

// Initialize all hardware components
void initializeHardware(void) {
    // Initialize GPIO
    gpio_init(LED_RED);
    gpio_init(LED_GREEN);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_set_dir(LED_BLUE, GPIO_OUT);
    
    // Initialize buttons with pull-up
    gpio_init(BTN_A);
    gpio_init(BTN_B);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_A);
    gpio_pull_up(BTN_B);
    
    // Initialize ADC for microphone
    adc_init();
    adc_gpio_init(MIC_IN);
    adc_select_input(2); // GPIO28 is on ADC2
    
    // Initialize I2C for OLED
    i2c_init(i2c1, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    ssd1306_init();
}

// Task to monitor noise levels
void vTaskMonitorNoise(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        uint16_t raw = adc_read();
        noiseLevel = raw;
        updateLEDStatus(raw);
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SAMPLE_RATE_MS));
    }
}

// Task to update the OLED display
void vTaskUpdateDisplay(void *pvParameters) {
    struct render_area area = {
        .start_column = 0,
        .end_column = ssd1306_width - 1,
        .start_page = 0,
        .end_page = ssd1306_n_pages - 1
    };
    calculate_render_area_buffer_length(&area);
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            // Clear buffer
            memset(displayBuffer, 0, sizeof(displayBuffer));
            
            // Draw static text
            ssd1306_draw_string(displayBuffer, 0, 0, "NOISE MONITOR");
            
            // Draw current level
            char levelStr[32];
            snprintf(levelStr, sizeof(levelStr), "Level: %d", noiseLevel);
            ssd1306_draw_string(displayBuffer, 0, 16, levelStr);
            
            // Draw thresholds
            snprintf(levelStr, sizeof(levelStr), "Warn: %d", warningThreshold);
            ssd1306_draw_string(displayBuffer, 0, 32, levelStr);
            snprintf(levelStr, sizeof(levelStr), "Dang: %d", dangerThreshold);
            ssd1306_draw_string(displayBuffer, 0, 48, levelStr);
            
            // Update display
            render_on_display(displayBuffer, &area);
            
            xSemaphoreGive(displayMutex);
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(DISPLAY_UPDATE_MS));
    }
}

// Update RGB LED based on noise level
void updateLEDStatus(int level) {
    if (level < warningThreshold) {
        // Green - OK
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 0);
    } else if (level < dangerThreshold) {
        // Yellow - Warning
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 1);
        gpio_put(LED_BLUE, 0);
    } else {
        // Red - Danger
        gpio_put(LED_RED, 1);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 0);
    }
}

// Task to handle joystick and button inputs
void vTaskHandleInput(void *pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Check if button A is pressed (decrease threshold)
        if (!gpio_get(BTN_A)) {
            warningThreshold -= 100;
            dangerThreshold -= 100;
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
        }
        
        // Check if button B is pressed (increase threshold)
        if (!gpio_get(BTN_B)) {
            warningThreshold += 100;
            dangerThreshold += 100;
            vTaskDelay(pdMS_TO_TICKS(200)); // Debounce
        }
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(50));
    }
}

int main() {
    // Initialize stdio
    stdio_init_all();
    
    // Initialize hardware
    initializeHardware();
    
    // Create mutex for display access
    displayMutex = xSemaphoreCreateMutex();
    
    // Create tasks
    xTaskCreate(vTaskMonitorNoise, "NoiseMonitor", configMINIMAL_STACK_SIZE, NULL, 2, NULL);
    xTaskCreate(vTaskUpdateDisplay, "DisplayUpdate", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vTaskHandleInput, "InputHandler", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    
    // Start the scheduler
    vTaskStartScheduler();
    
    // We should never get here
    while (1) {};
    
    return 0;
}