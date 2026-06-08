#pragma once
#include <stdbool.h>

typedef enum {
    WIFI_OK       =  0,
    WIFI_ERR_FAIL = -1,
} wifi_result_t;

// Call led_init() before wifi_connect() — led_init() handles cyw43_arch_init().
wifi_result_t wifi_connect(const char *ssid, const char *password);
bool          wifi_is_connected(void);
const char   *wifi_get_ip(void);
void          wifi_poll(void);  // call each loop iteration in poll mode
