#ifndef TEST_LWIP_MOCK_H
#define TEST_LWIP_MOCK_H
#include <stdint.h>

typedef int err_t;
typedef uint16_t u16_t;
typedef struct { uint32_t address; } ip_addr_t;
#define ERR_OK 0
#define ERR_MEM -1
#define ERR_ARG -2
#define ERR_ALREADY -3
#define ERR_IF -4
#define IP_GET_TYPE(addr) 0
#define ip_addr_isany(addr) ((addr)->address == 0)
#define ip_addr_cmp(a, b) ((a)->address == (b)->address)
#define PBUF_TRANSPORT 0
#define PBUF_RAM 0

struct pbuf {
    uint16_t tot_len;
    uint8_t *payload;
};
struct udp_pcb;
typedef void (*udp_recv_fn)(void *, struct udp_pcb *, struct pbuf *, const ip_addr_t *, u16_t);
typedef void (*sys_timeout_handler)(void *);
struct udp_pcb *udp_new_ip_type(uint8_t type);
void udp_remove(struct udp_pcb *pcb);
void udp_recv(struct udp_pcb *pcb, udp_recv_fn fn, void *arg);
err_t udp_sendto(struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *address, u16_t port);
struct pbuf *pbuf_alloc(int layer, uint16_t length, int type);
uint8_t pbuf_free(struct pbuf *p);
err_t pbuf_take(struct pbuf *p, const void *data, uint16_t length);
uint16_t pbuf_copy_partial(const struct pbuf *p, void *data, uint16_t length, uint16_t offset);
uint32_t sys_now(void);
void sys_timeout(uint32_t ms, sys_timeout_handler fn, void *arg);
void sys_untimeout(sys_timeout_handler fn, void *arg);
#endif
