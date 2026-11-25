#include <inttypes.h>
#include <stdio.h>

#include "bic/ntp.h"
#include "lwip/dns.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#define NTP_SERVER "pool.ntp.org"

static bic_ntp_client_t client = BIC_NTP_CLIENT_INIT;
static bool finished;

static void ntp_done(void *arg, bic_ntp_status_t status, const bic_ntp_time_t *time) {
    (void)arg;
    if (status == BIC_NTP_SUCCESS) {
        printf("Unix time: %" PRIu64 " seconds, fraction: 0x%08" PRIx32 "\n",
               time->unix_seconds, time->fraction);
    } else {
        printf("NTP request timed out\n");
    }
    finished = true;
}

static void start_request(const ip_addr_t *address) {
    if (!address) {
        printf("NTP server DNS lookup failed\n");
        finished = true;
        return;
    }
    err_t error = bic_ntp_request(&client, address, 10000, ntp_done, NULL);
    if (error != ERR_OK) {
        printf("Could not start NTP request: %d\n", (int)error);
        finished = true;
    }
}

static void dns_found(const char *hostname, const ip_addr_t *address, void *arg) {
    (void)hostname;
    (void)arg;
    start_request(address);
}

int main(void) {
    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("Wi-Fi initialization failed\n");
        return 1;
    }
    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Wi-Fi connection failed\n");
        cyw43_arch_deinit();
        return 1;
    }

    ip_addr_t address;
    cyw43_arch_lwip_begin();
    err_t error = dns_gethostbyname(NTP_SERVER, &address, dns_found, NULL);
    if (error == ERR_OK) {
        start_request(&address);
    } else if (error != ERR_INPROGRESS) {
        printf("Could not start DNS lookup: %d\n", (int)error);
        finished = true;
    }
    cyw43_arch_lwip_end();

    while (!finished) {
        cyw43_arch_poll();
        sleep_ms(10);
    }
    cyw43_arch_deinit();
    return 0;
}
