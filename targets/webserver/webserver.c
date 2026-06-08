#include "wifi.h"
#include "serial.h"
#include "secrets.h"
#include "pico/stdlib.h"
#include "lwip/apps/httpd.h"

#define REPORT_MS 5000

static const char *cgi_status(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
    (void)iIndex; (void)iNumParams; (void)pcParam; (void)pcValue;
    return "/status.txt";
}

static const tCGI cgi_handlers[] = {
    { "/status", cgi_status },
};

int main() {
    serial_init();

    serial_println("connecting to %s ...", WIFI_SSID);
    if (wifi_connect(WIFI_SSID, WIFI_PASSWORD) != WIFI_OK) {
        serial_println("wifi failed");
        return 1;
    }
    serial_println("ip: %s", wifi_get_ip());

    httpd_init();
    http_set_cgi_handlers(cgi_handlers, 1);
    serial_println("http ready at http://%s/", wifi_get_ip());

    absolute_time_t next_report = make_timeout_time_ms(REPORT_MS);
    uint32_t uptime_s = 0;

    while (true) {
        wifi_poll();

        if (time_reached(next_report)) {
            uptime_s += REPORT_MS / 1000;
            serial_println("uptime: %lu s", uptime_s);
            next_report = make_timeout_time_ms(REPORT_MS);
        }
    }
}
