#include "wifi.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"

wifi_result_t wifi_connect(const char *ssid, const char *password) {
    if (cyw43_arch_init() != 0) return WIFI_ERR_FAIL;
    cyw43_arch_enable_sta_mode();
    int err = cyw43_arch_wifi_connect_timeout_ms(
        ssid, password, CYW43_AUTH_WPA2_AES_PSK, 10000);
    return err == 0 ? WIFI_OK : WIFI_ERR_FAIL;
}

bool wifi_is_connected(void) {
    return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_JOIN;
}

const char *wifi_get_ip(void) {
    return ip4addr_ntoa(netif_ip4_addr(netif_list));
}

void wifi_poll(void) {
    cyw43_arch_poll();
}

void wifi_led_on(void) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}

void wifi_led_off(void) {
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}

void wifi_led_toggle(void) {
    static bool state = false;
    state = !state;
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state);
}
