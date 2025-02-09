#include "noise_monitor.h"
#include "peripherals.h"

#include <stdlib.h>

int noise_level = 0;
int warning_threshold = NOISE_THRESHOLD_WARNING;
int danger_threshold = NOISE_THRESHOLD_DANGER;
int threshold_gap = DEFAULT_GAP;

void vTaskMonitorNoise(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1)
    {
        adc_select_input(2);
        adc_fifo_drain();
        adc_run(false);

        dma_channel_configure(dma_channel, &dma_cfg,
                              adc_buffer,
                              &adc_hw->fifo,
                              SAMPLES,
                              true);

        adc_run(true);
        dma_channel_wait_for_finish_blocking(dma_channel);
        adc_run(false);

        float rms = calculate_rms(adc_buffer, SAMPLES);
        noise_level = (int)(rms * 1000);

        update_led_status(noise_level);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SAMPLE_RATE_MS));
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
