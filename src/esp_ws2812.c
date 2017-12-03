#include <string.h>
#include <stdlib.h>
// #include <math.h>

#include <FreeRTOS.h>
#include <task.h>

#include <ws2812_i2s/ws2812_i2s.h>

#include "esp_ws2812.h"

#define LEDS_COUNT  20
#define LEDS_SATURATION 160
#define LEDS_LUMINANCE 30

#define HUE_STEP 5

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} ws2812_rgb_t;

ws2812_pixel_t pixels[LEDS_COUNT];

// http://www.ganssle.com/approx.htm
#define PI 3.1415926535897932384626433    // PI
double const twopi = 2.0 * PI;          // PI times 2
double const two_over_pi = 2.0 / PI;       // 2/PI

float cos_32s(float x)
{
    const float c1= 0.99940307;
    const float c2=-0.49558072;
    const float c3= 0.03679168;

    float x2;                           // The input argument squared

    x2=x * x;
    return (c1 + x2*(c2 + c3 * x2));
}

float cos_32(float x){
    int quad;                       // what quadrant are we in?

    // x = fmod(x, twopi);               // Get rid of values > 2* PI
    x = x - (int)(x / twopi) * twopi;               // Get rid of values > 2* PI
    if (x < 0) x=-x;                    // cos(-x) = cos(x)
    quad = (int)(x * two_over_pi);          // Get quadrant # (0 to 3) we're in
    switch (quad){
        case 0: return  cos_32s(x);
        case 1: return -cos_32s(PI-x);
        case 2: return -cos_32s(x-PI);
        case 3: return  cos_32s(twopi-x);
    }
}

void hsv_to_rgb(uint8_t hsv_h, uint8_t hsv_s, uint8_t hsv_v, ws2812_rgb_t *rgb) {
    int i;
    uint8_t sector, frac, p, q, t;

    if( hsv_s == 0 ) {  // achromatic
        rgb->red = rgb->green = rgb->blue = hsv_v;
        return;
    }

    sector = hsv_v / 43;
    frac = (hsv_v - (sector * 43)) * 6;

    p = (hsv_v * (255 - hsv_s)) >> 8;
    q = (hsv_v * (255 - ((hsv_s * frac) >> 8))) >> 8;
    t = (hsv_v * (255 - ((hsv_s * (255 - frac)) >> 8))) >> 8;

    switch( i ) {
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

void hsv_to_rgb_old(uint8_t hsv_h, uint8_t hsv_s, uint8_t hsv_v, ws2812_rgb_t *rgb) {
    if (hsv_s == 0) {   // achromatic
        rgb->red = rgb->green = rgb->blue = hsv_v;
    } else {
        float hsvf_h = (float)hsv_h / 360;
        float hsvf_s = (float)hsv_s / 255;
        float hsvf_v = (float)hsv_v / 255;
        rgb->red = 255 * hsvf_v * (1 + hsvf_s * (cos_32(hsvf_h) - 1));
        rgb->green = 255 * hsvf_v * (1 + hsvf_s * (cos_32(hsvf_h - 2.09439) - 1));
        rgb->blue = 255 * hsvf_v * (1 + hsvf_s * (cos_32(hsvf_h + 2.09439) - 1));
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
    //printf("hue:%i, rgb: %i:%i:%i\n", hue_val, ws2812_rgb.red, ws2812_rgb.green, ws2812_rgb.blue);
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
        //printf("cur:%i, dst:%i\n", hue_cur, hue_dst);
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

