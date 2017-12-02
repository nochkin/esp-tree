#include <esp/uart.h>

#include "web.h"
#include "esp_ws2812.h"

void user_init() {
    uart_set_baud(0, 115200);

    web_init();

    esp_ws2812_init();
}

