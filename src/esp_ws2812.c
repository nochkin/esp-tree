#include <string.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include <ws2812_i2s/ws2812_i2s.h>

#include "esp_ws2812.h"

#define LEDS_COUNT  50
#define LEDS_SATURATION 160
#define LEDS_LUMINANCE 20

#define HUE_STEP 3

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ws2812_rgb_t;

ws2812_pixel_t pixels[LEDS_COUNT];

void hsv_to_rgb(uint8_t hsv_h, uint8_t hsv_s, uint8_t hsv_v, ws2812_rgb_t *rgb) {
    uint8_t sector, frac, p, q, t;

    if (hsv_s == 0) {  // achromatic
        rgb->red = rgb->green = rgb->blue = hsv_v;
        return;
    }

    sector = hsv_h / 43;
    frac = (hsv_h - (sector * 43)) * 6;

    p = (hsv_v * (255 - hsv_s)) >> 8;
    q = (hsv_v * (255 - ((hsv_s * frac) >> 8))) >> 8;
    t = (hsv_v * (255 - ((hsv_s * (255 - frac)) >> 8))) >> 8;

    switch (sector) {
        case 0:
            rgb->red = hsv_v;
            rgb->green = t;
            rgb->blue = p;
            break;
        case 1:
            rgb->red = q;
            rgb->green = hsv_v;
            rgb->blue = p;
            break;
        case 2:
            rgb->red = p;
            rgb->green = hsv_v;
            rgb->blue = t;
            break;
        case 3:
            rgb->red = p;
            rgb->green = q;
            rgb->blue = hsv_v;
            break;
        case 4:
            rgb->red = t;
            rgb->green = p;
            rgb->blue = hsv_v;
            break;
        default:        // case 5:
            rgb->red = hsv_v;
            rgb->green = p;
            rgb->blue = q;
            break;
    }
}

void push_led(uint8_t hue_val) {
    ws2812_rgb_t ws2812_rgb;

    for (int ii=0; ii<(LEDS_COUNT - 1); ii++) {
        pixels[ii].red = pixels[ii + 1].red;
        pixels[ii].green = pixels[ii + 1].green;
        pixels[ii].blue = pixels[ii + 1].blue;
    }

    hsv_to_rgb(hue_val, LEDS_SATURATION, LEDS_LUMINANCE, &ws2812_rgb);
    // remap colors
    pixels[LEDS_COUNT - 1].red = ws2812_rgb.green;
    pixels[LEDS_COUNT - 1].green = ws2812_rgb.blue;
    pixels[LEDS_COUNT - 1].blue = ws2812_rgb.red;
}

void esp_ws2812(void *pvParameters) {
    ws2812_i2s_init(LEDS_COUNT, PIXEL_RGB);
    memset(pixels, 0, sizeof(ws2812_pixel_t) * LEDS_COUNT);

    uint8_t hue_cur = 0;
    uint8_t hue_dst = 0;
    int8_t hue_step_sign = 1;

    while (1) {
        if (abs(hue_cur - hue_dst) < HUE_STEP) {
            hue_dst = rand() % 255;
        }
        hue_step_sign = (hue_cur < hue_dst) ? 1 : -1;
        hue_cur += HUE_STEP * hue_step_sign;

        push_led(hue_cur);

        ws2812_i2s_update(pixels, PIXEL_RGB);

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void esp_ws2812_init() {
    xTaskCreate(&esp_ws2812, "ws2812", 256, NULL, 10, NULL);
}

