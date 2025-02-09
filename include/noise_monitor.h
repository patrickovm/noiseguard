#ifndef NOISE_MONITOR_H
#define NOISE_MONITOR_H

#include "FreeRTOS.h"
#include "task.h"

#define DEFAULT_GAP 1000
#define MIN_GAP 200
#define MAX_GAP 2000

#define NOISE_THRESHOLD_WARNING 2000
#define NOISE_THRESHOLD_DANGER 3000
#define SAMPLE_RATE_MS 100

void vTaskMonitorNoise(void *pvParameters);

void update_led_status(int level);

extern int noise_level;
extern int warning_threshold;
extern int danger_threshold;
extern int threshold_gap;

#endif // NOISE_MONITOR_H