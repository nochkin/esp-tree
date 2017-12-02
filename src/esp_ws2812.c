#include <string.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include <ws2812_i2s/ws2812_i2s.h>

#include "esp_ws2812.h"

#define LEDS_COUNT  50
#define LEDS_BRIGHTNESS 30

void esp_ws2812(void *pvParameters) {
    ws2812_pixel_t pixels[LEDS_COUNT];
    ws2812_i2s_init(LEDS_COUNT, PIXEL_RGB);
    memset(pixels, 0, sizeof(ws2812_pixel_t) * LEDS_COUNT);

    while (1) {
        int ii = rand() % LEDS_COUNT;
        pixels[ii].red = rand() % LEDS_BRIGHTNESS; // green
        pixels[ii].green = rand() % LEDS_BRIGHTNESS; // blue
        pixels[ii].blue = rand() % LEDS_BRIGHTNESS; // red
        ws2812_i2s_update(pixels, PIXEL_RGB);

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void esp_ws2812_init() {
    xTaskCreate(&esp_ws2812, "ws2812", 256, NULL, 10, NULL);
}

