#include "common.h"
#include "mf.h"
#include "stm32h7xx_hal.h"
#include <assert.h>
#include <lwip/err.h>
#include <lwip/tcp.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  struct tcp_pcb *pcb;
  set(uintptr_t) active_client_pcb;
} HttpServerWidgetState_t;

static HttpServerWidgetState_t _state = {0};

static void close_pcb(struct tcp_pcb *pcb,
                      HttpServerWidgetState_t *const state) {
  tcp_sent(pcb, NULL);
  tcp_recv(pcb, NULL);
  err_t status = tcp_close(pcb);
  if (unlikely(status != ERR_OK)) {
    tcp_abort(pcb);
  }
  erase(&state->active_client_pcb, (uintptr_t)pcb);
}

static void on_err(void *ctx, err_t err) {
  UNUSED(ctx);
  UNUSED(err);
}

static err_t on_sent(void *ctx, struct tcp_pcb *pcb, u16_t len) {
  // HttpServerWidgetState_t *const state = (HttpServerWidgetState_t *)ctx;
  // close_pcb(pcb, state);
  return ERR_OK;
}

static err_t on_recv(void *ctx, struct tcp_pcb *pcb, struct pbuf *p,
                     err_t err) {
  HttpServerWidgetState_t *const state = (HttpServerWidgetState_t *)ctx;
  if (p != NULL) {
    if (likely(pcb != NULL)) {
      tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
  }
  if (likely(pcb != NULL)) {
    if (p != NULL) {
      const char *response = "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/html\r\n"
                             "\r\n"
                             "Http Server";

      tcp_write(pcb, response, strlen(response), TCP_WRITE_FLAG_COPY);
      close_pcb(pcb, state);
    } else { // p == NULL -> FIN
      close_pcb(pcb, state);
    }
  }
  return ERR_OK;
}

static err_t on_accept(void *ctx, struct tcp_pcb *pcb, err_t err) {
  HttpServerWidgetState_t *const state = (HttpServerWidgetState_t *)ctx;
  if (likely(pcb != NULL)) {
    if (likely(err == ERR_OK)) {
      tcp_arg(pcb, ctx);
      tcp_err(pcb, on_err);
      tcp_sent(pcb, on_sent);
      tcp_recv(pcb, on_recv);
      insert(&state->active_client_pcb, (uintptr_t)pcb);
    } else {
      tcp_abort(pcb);
    }
  }
  return ERR_OK;
}

static void *init_state(mContext_t *context) {
  HttpServerWidgetState_t *const state = &_state;
  init(&state->active_client_pcb);

  struct tcp_pcb *pcb = tcp_new();
  assert(pcb != NULL);
  err_t err = tcp_bind(pcb, IP_ADDR_ANY, 80);
  assert(err == ERR_OK);
  pcb = tcp_listen(pcb);
  assert(pcb != NULL);
  tcp_arg(pcb, state);
  tcp_accept(pcb, on_accept);
  state->pcb = pcb;

  return state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  return;
}

static void dispose(mContext_t *context) {
  m_get_state_cast(state, context, HttpServerWidgetState_t);
  for_each(&state->active_client_pcb, el) tcp_abort((struct tcp_pcb *)(*el));
  cleanup(&state->active_client_pcb);
  tcp_close(
      state->pcb); // a listened tcp pcb only can call tcp_close, not tcp_abort
  state->pcb = NULL;
  return;
};

const mWidgetClass_t HttpServerWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .should_rebuild = m_always_rebuild,
    .dispose = &dispose,
};
