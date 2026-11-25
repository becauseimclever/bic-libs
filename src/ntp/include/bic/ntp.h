#ifndef BIC_NTP_H
#define BIC_NTP_H

#include <stdint.h>
#include "lwip/err.h"
#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BIC_NTP_SUCCESS,
    BIC_NTP_TIMEOUT
} bic_ntp_status_t;

typedef struct {
    uint64_t unix_seconds;
    uint32_t fraction;
} bic_ntp_time_t;

typedef void (*bic_ntp_callback_t)(void *arg, bic_ntp_status_t status,
                                   const bic_ntp_time_t *time);

typedef struct {
    struct udp_pcb *pcb;
    ip_addr_t server;
    uint8_t token[8];
    bic_ntp_callback_t callback;
    void *callback_arg;
} bic_ntp_client_t;

#define BIC_NTP_CLIENT_INIT {0}

/*
 * Call from the lwIP core context, or with the port's lwIP lock held.
 * The client must be zero-initialized and remain alive until completion or
 * cancellation. A successful start invokes callback once, asynchronously.
 * Immediate errors are returned and do not invoke callback.
 */
err_t bic_ntp_request(bic_ntp_client_t *client, const ip_addr_t *server,
                      uint32_t timeout_ms, bic_ntp_callback_t callback, void *arg);

/* Cancel an active request without invoking its callback. */
void bic_ntp_cancel(bic_ntp_client_t *client);

#ifdef __cplusplus
}
#endif

#endif
