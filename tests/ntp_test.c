#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "bic/ntp.h"
#include "lwip/mock.h"

struct udp_pcb { udp_recv_fn receive; void *arg; };
static struct udp_pcb pcb;
static uint8_t sent[48];
static sys_timeout_handler timer;
static void *timer_arg;
static int pcb_in_use;
static int fail_alloc;
static err_t send_error;
static int callbacks;
static bic_ntp_status_t last_status;
static bic_ntp_time_t last_time;

struct udp_pcb *udp_new_ip_type(uint8_t type) {
    (void)type;
    if (pcb_in_use) return NULL;
    pcb_in_use = 1;
    memset(&pcb, 0, sizeof(pcb));
    return &pcb;
}
void udp_remove(struct udp_pcb *p) { assert(p == &pcb); pcb_in_use = 0; }
void udp_recv(struct udp_pcb *p, udp_recv_fn fn, void *arg) { p->receive = fn; p->arg = arg; }
err_t udp_sendto(struct udp_pcb *p, struct pbuf *buf, const ip_addr_t *addr, u16_t port) {
    (void)p; (void)addr;
    assert(port == 123 && buf->tot_len == sizeof(sent));
    memcpy(sent, buf->payload, sizeof(sent));
    return send_error;
}
struct pbuf *pbuf_alloc(int layer, uint16_t length, int type) {
    (void)layer; (void)type;
    if (fail_alloc) return NULL;
    struct pbuf *p = malloc(sizeof(*p));
    assert(p);
    p->payload = malloc(length);
    assert(p->payload);
    p->tot_len = length;
    return p;
}
uint8_t pbuf_free(struct pbuf *p) { free(p->payload); free(p); return 1; }
err_t pbuf_take(struct pbuf *p, const void *data, uint16_t length) {
    memcpy(p->payload, data, length);
    return ERR_OK;
}
uint16_t pbuf_copy_partial(const struct pbuf *p, void *data, uint16_t length, uint16_t offset) {
    if (offset >= p->tot_len) return 0;
    if (length > p->tot_len - offset) length = p->tot_len - offset;
    memcpy(data, p->payload + offset, length);
    return length;
}
uint32_t sys_now(void) { return 12345; }
void sys_timeout(uint32_t ms, sys_timeout_handler fn, void *arg) {
    assert(ms && !timer);
    timer = fn;
    timer_arg = arg;
}
void sys_untimeout(sys_timeout_handler fn, void *arg) {
    if (timer == fn && timer_arg == arg) { timer = NULL; timer_arg = NULL; }
}
static void result(void *arg, bic_ntp_status_t status, const bic_ntp_time_t *time) {
    assert(arg == &callbacks);
    callbacks++;
    last_status = status;
    if (time) last_time = *time;
    else assert(status == BIC_NTP_TIMEOUT);
}
static void put32(uint8_t *p, uint32_t n) {
    p[0] = (uint8_t)(n >> 24); p[1] = (uint8_t)(n >> 16);
    p[2] = (uint8_t)(n >> 8); p[3] = (uint8_t)n;
}
static void deliver(const uint8_t *data, uint16_t size, const ip_addr_t *address, uint16_t port) {
    struct pbuf *p = pbuf_alloc(0, size, 0);
    memcpy(p->payload, data, size);
    pcb.receive(pcb.arg, &pcb, p, address, port);
}
static void response(uint8_t *packet) {
    memset(packet, 0, 48);
    packet[0] = 0x24;
    packet[1] = 2;
    memcpy(packet + 24, sent + 40, 8);
    put32(packet + 40, 2208988801UL);
    put32(packet + 44, 0x80000000UL);
}
int main(void) {
    bic_ntp_client_t client = BIC_NTP_CLIENT_INIT;
    ip_addr_t server = {1}, other = {2}, any = {0};
    uint8_t packet[48];

    assert(bic_ntp_request(NULL, &server, 100, result, &callbacks) == ERR_ARG);
    assert(bic_ntp_request(&client, &any, 100, result, &callbacks) == ERR_ARG);
    assert(bic_ntp_request(&client, &server, 0, result, &callbacks) == ERR_ARG);
    assert(bic_ntp_request(&client, &server, 0x80000000UL, result, &callbacks) == ERR_ARG);
    fail_alloc = 1;
    assert(bic_ntp_request(&client, &server, 100, result, &callbacks) == ERR_MEM);
    assert(!pcb_in_use && !timer);
    fail_alloc = 0;
    send_error = ERR_IF;
    assert(bic_ntp_request(&client, &server, 100, result, &callbacks) == ERR_IF);
    assert(!pcb_in_use && !timer);
    send_error = ERR_OK;

    assert(bic_ntp_request(&client, &server, 100, result, &callbacks) == ERR_OK);
    assert(sent[0] == 0x23 && timer);
    assert(bic_ntp_request(&client, &server, 100, result, &callbacks) == ERR_ALREADY);
    response(packet);
    deliver(packet, 47, &server, 123);
    deliver(packet, 48, &other, 123);
    deliver(packet, 48, &server, 124);
    packet[0] = 0xe4; deliver(packet, 48, &server, 123); packet[0] = 0x24;
    packet[0] = 0x2c; deliver(packet, 48, &server, 123); packet[0] = 0x24;
    packet[0] = 0x23; deliver(packet, 48, &server, 123); packet[0] = 0x24;
    packet[1] = 0; deliver(packet, 48, &server, 123); packet[1] = 16;
    deliver(packet, 48, &server, 123); packet[1] = 2;
    packet[24] ^= 1; deliver(packet, 48, &server, 123); packet[24] ^= 1;
    memset(packet + 40, 0, 8); deliver(packet, 48, &server, 123);
    response(packet);
    assert(callbacks == 0 && timer && pcb_in_use);
    deliver(packet, 48, &server, 123);
    assert(callbacks == 1 && last_status == BIC_NTP_SUCCESS);
    assert(last_time.unix_seconds == 1 && last_time.fraction == 0x80000000UL);
    assert(!timer && !pcb_in_use);

    assert(bic_ntp_request(&client, &server, 100, result, &callbacks) == ERR_OK);
    deliver(packet, 48, &server, 123);
    assert(callbacks == 1 && timer);
    response(packet);
    put32(packet + 40, 0);
    put32(packet + 44, 1);
    deliver(packet, 48, &server, 123);
    assert(callbacks == 2 && last_time.unix_seconds == UINT64_C(2085978496));
    assert(last_time.fraction == 1);

    assert(bic_ntp_request(&client, &server, 100, result, &callbacks) == ERR_OK);
    sys_timeout_handler expired = timer;
    void *expired_arg = timer_arg;
    timer = NULL;
    expired(expired_arg);
    assert(callbacks == 3 && last_status == BIC_NTP_TIMEOUT && !pcb_in_use);
    assert(bic_ntp_request(&client, &server, 100, result, &callbacks) == ERR_OK);
    bic_ntp_cancel(&client);
    assert(callbacks == 3 && !timer && !pcb_in_use);
    bic_ntp_cancel(&client);
    return 0;
}
