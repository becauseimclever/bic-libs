#include "bic/ntp.h"

#include <string.h>

#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"

#define NTP_PACKET_SIZE 48
#define NTP_PORT 123
#define NTP_UNIX_DELTA 2208988800UL

static void ntp_timeout(void *arg);

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

static void write_be32(uint8_t *p, uint32_t value) {
    p[0] = (uint8_t)(value >> 24);
    p[1] = (uint8_t)(value >> 16);
    p[2] = (uint8_t)(value >> 8);
    p[3] = (uint8_t)value;
}

void bic_ntp_cancel(bic_ntp_client_t *client) {
    if (client && client->pcb) {
        sys_untimeout(ntp_timeout, client);
        udp_remove(client->pcb);
        client->pcb = NULL;
        client->callback = NULL;
        client->callback_arg = NULL;
    }
}

static void ntp_complete(bic_ntp_client_t *client, bic_ntp_status_t status,
                         const bic_ntp_time_t *time) {
    bic_ntp_callback_t callback = client->callback;
    void *arg = client->callback_arg;
    bic_ntp_cancel(client);
    callback(arg, status, time);
}

static void ntp_timeout(void *arg) {
    ntp_complete((bic_ntp_client_t *)arg, BIC_NTP_TIMEOUT, NULL);
}

static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                     const ip_addr_t *addr, u16_t port) {
    bic_ntp_client_t *client = (bic_ntp_client_t *)arg;
    uint8_t packet[NTP_PACKET_SIZE];
    int valid = p && addr && port == NTP_PORT &&
                ip_addr_cmp(addr, &client->server) &&
                p->tot_len >= NTP_PACKET_SIZE &&
                pbuf_copy_partial(p, packet, sizeof(packet), 0) == sizeof(packet);
    (void)pcb;
    if (p) {
        pbuf_free(p);
    }
    if (!valid || (packet[0] >> 6) == 3 ||
        ((packet[0] >> 3) & 7) < 3 || ((packet[0] >> 3) & 7) > 4 ||
        (packet[0] & 7) != 4 || packet[1] == 0 || packet[1] > 15 ||
        memcmp(packet + 24, client->token, sizeof(client->token)) != 0 ||
        (read_be32(packet + 40) == 0 && read_be32(packet + 44) == 0)) {
        return;
    }

    uint32_t seconds = read_be32(packet + 40);
    bic_ntp_time_t time;
    time.unix_seconds = seconds >= NTP_UNIX_DELTA
                            ? (uint64_t)seconds - NTP_UNIX_DELTA
                            : (uint64_t)seconds + (UINT64_C(1) << 32) - NTP_UNIX_DELTA;
    time.fraction = read_be32(packet + 44);
    ntp_complete(client, BIC_NTP_SUCCESS, &time);
}

err_t bic_ntp_request(bic_ntp_client_t *client, const ip_addr_t *server,
                      uint32_t timeout_ms, bic_ntp_callback_t callback, void *arg) {
    static uint32_t sequence;
    uint8_t packet[NTP_PACKET_SIZE] = {0};
    struct pbuf *p;
    err_t error;

    if (!client || !server || !callback || !timeout_ms || timeout_ms > INT32_MAX ||
        ip_addr_isany(server)) {
        return ERR_ARG;
    }
    if (client->pcb) {
        return ERR_ALREADY;
    }
    client->pcb = udp_new_ip_type(IP_GET_TYPE(server));
    if (!client->pcb) {
        return ERR_MEM;
    }
    client->server = *server;
    client->callback = callback;
    client->callback_arg = arg;
    packet[0] = 0x23; /* NTP version 4, client mode. */
    write_be32(client->token, sys_now());
    write_be32(client->token + 4, ++sequence);
    memcpy(packet + 40, client->token, sizeof(client->token));

    p = pbuf_alloc(PBUF_TRANSPORT, sizeof(packet), PBUF_RAM);
    if (!p) {
        bic_ntp_cancel(client);
        return ERR_MEM;
    }
    error = pbuf_take(p, packet, sizeof(packet));
    if (error == ERR_OK) {
        udp_recv(client->pcb, ntp_recv, client);
        error = udp_sendto(client->pcb, p, server, NTP_PORT);
    }
    pbuf_free(p);
    if (error != ERR_OK) {
        bic_ntp_cancel(client);
        return error;
    }
    sys_timeout(timeout_ms, ntp_timeout, client);
    return ERR_OK;
}
