#include "peripherals.h"
#include "ssd1306.h"

#include <math.h>

uint16_t adc_buffer[SAMPLES];
uint dma_channel;
dma_channel_config dma_cfg;

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
    adc_gpio_init(JOYSTICK_X);

    i2c_init(i2c1, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_init();

    adc_fifo_setup(true, true, 1, false, false);
    adc_set_clkdiv(96.0f);

    dma_channel = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_cfg, false);
    channel_config_set_write_increment(&dma_cfg, true);
    channel_config_set_dreq(&dma_cfg, DREQ_ADC);
}

int read_joystick_x(void)
{
    adc_select_input(1);
    return adc_read();
}

float calculate_rms(uint16_t *buffer, int samples)
{
    float sum = 0.0f;
    for (int i = 0; i < samples; i++)
    {
        float voltage = buffer[i] * 3.3f / (1 << 12);
        sum += voltage * voltage;
    }
    return sqrt(sum / samples);
}