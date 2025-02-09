#ifndef PERIPHERALS_H
#define PERIPHERALS_H

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"

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

#define SAMPLES 200

void init_peripherals(void);
int read_joystick_x(void);
float calculate_rms(uint16_t *buffer, int samples);

extern uint16_t adc_buffer[];
extern uint dma_channel;
extern dma_channel_config dma_cfg;

#endif // PERIPHERALS_H