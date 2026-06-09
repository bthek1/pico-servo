#pragma once
#include <stdbool.h>

typedef enum {
    WIFI_OK       =  0,
    WIFI_ERR_FAIL = -1,
} wifi_result_t;

wifi_result_t wifi_connect(const char *ssid, const char *password);
bool          wifi_is_connected(void);
const char   *wifi_get_ip(void);
void          wifi_poll(void);      // call each loop iteration in poll mode
// Call after wifi_connect to override DHCP with a fixed address
void          wifi_set_static_ip(const char *ip, const char *mask, const char *gw);

void          wifi_led_on(void);
void          wifi_led_off(void);
void          wifi_led_toggle(void);
