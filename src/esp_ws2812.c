#include <string.h>
#include <stdlib.h>

#include <espressif/esp_common.h>
#include <esp/hwrand.h>

#include <FreeRTOS.h>
#include <task.h>

#include <ws2812_i2s/ws2812_i2s.h>

#include "esp_ws2812.h"

#define LEDS_COUNT  50
#define LEDS_SATURATION 160
#define LEDS_LUMINANCE 40

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

void set_pixel_color(uint16_t pixel_idx, uint8_t hue_val, uint8_t lum_val) {
    ws2812_rgb_t ws2812_rgb;
    hsv_to_rgb(hue_val, LEDS_SATURATION, lum_val, &ws2812_rgb);
    // remap colors
    pixels[pixel_idx].red = ws2812_rgb.green;
    pixels[pixel_idx].green = ws2812_rgb.blue;
    pixels[pixel_idx].blue = ws2812_rgb.red;
}

void set_pixels_color(uint8_t hue_val, uint8_t lum_val) {
    ws2812_rgb_t ws2812_rgb;
    hsv_to_rgb(hue_val, LEDS_SATURATION, lum_val, &ws2812_rgb);
    for (uint16_t num=0; num<LEDS_COUNT; num++) {
        // remap colors
        pixels[num].red = ws2812_rgb.green;
        pixels[num].green = ws2812_rgb.blue;
        pixels[num].blue = ws2812_rgb.red;
    }
}

void rainbow(uint32_t *pos) {
    for (uint16_t num=0; num<LEDS_COUNT; num++) {
        set_pixel_color(num, (*pos + num) & 0xff, LEDS_LUMINANCE);
    }
    *pos += 1;
}

void push_led(uint8_t hue_val, uint8_t lum_val) {
    ws2812_rgb_t ws2812_rgb;

    for (int ii=0; ii<(LEDS_COUNT - 1); ii++) {
        pixels[ii].red = pixels[ii + 1].red;
        pixels[ii].green = pixels[ii + 1].green;
        pixels[ii].blue = pixels[ii + 1].blue;
    }

    set_pixel_color(LEDS_COUNT - 1, hue_val, lum_val);
}

static uint8_t hue_cur = 0;
static uint8_t hue_dst = 0;

void falling_color(uint32_t *pos) {
    if (abs(hue_cur - hue_dst) < HUE_STEP) {
        hue_dst = hwrand() % 255;
    }
    int8_t hue_step_sign = (hue_cur < hue_dst) ? 1 : -1;
    hue_cur += HUE_STEP * hue_step_sign;

    push_led(hue_cur, LEDS_LUMINANCE);
}

void running_color(uint32_t *pos) {
    uint8_t run_len = 7;
    uint8_t lum_val = (*pos >= run_len) ? 0 : ((LEDS_LUMINANCE / run_len) * (run_len - *pos));
    push_led(hue_cur, lum_val);
    *pos += 1;
    if (*pos > (LEDS_COUNT / 2)) {
        *pos = 0;
        hue_cur = hwrand() % 255;
    }
}

void running_single_color(uint32_t *pos) {
    if (*pos == 0) {
        push_led(hue_cur, LEDS_LUMINANCE);
    } else {
        push_led(hue_cur, 0);
    }
    *pos += 1;
    if (*pos > (LEDS_COUNT / 3)) {
        *pos = 0;
        hue_cur = hwrand() % 255;
    }
}

void blink(uint32_t *pos) {
    if (*pos == 0) *pos = hwrand() % 255;
    set_pixels_color((20 - hwrand() % 40) + *pos, LEDS_LUMINANCE);
}

void leds_off(uint32_t *pos) {
    set_pixels_color(0, 0);
}

void esp_ws2812(void *pvParameters) {
    ws2812_i2s_init(LEDS_COUNT, PIXEL_RGB);
    memset(pixels, 0, sizeof(ws2812_pixel_t) * LEDS_COUNT);

    uint32_t pos = 0;
    uint8_t fx = 0;
    uint8_t fx_count = 5;

    uint32_t my_time = sdk_system_get_time();
    uint32_t delay = 20;

    while (1) {
        switch(fx) {
            case 0:
                delay = 40;
                falling_color(&pos);
                break;
            case 1:
                delay = 20;
                rainbow(&pos);
                break;
            case 2:
                delay = 1000;
                blink(&pos);
                break;
            case 3:
                delay = 100;
                running_color(&pos);
                break;
            case 4:
                delay = 100;
                running_single_color(&pos);
                break;
            default:
                delay = 1000;
                leds_off(&pos);
                break;
        }

        ws2812_i2s_update(pixels, PIXEL_RGB);

        vTaskDelay(delay / portTICK_PERIOD_MS);

        if ((sdk_system_get_time() - my_time) > 10 * 1000000) {
            fx = (fx + 1) % fx_count;
            pos = 0;
            hue_cur = hwrand() % 255;
            my_time = sdk_system_get_time();
        }
    }
}

void esp_ws2812_init() {
    xTaskCreate(&esp_ws2812, "ws2812", 256, NULL, 10, NULL);
}

