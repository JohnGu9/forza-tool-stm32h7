#include "widgets.h"

#include "mf.h"

#include <assert.h>
#include <lwip/err.h>
#include <lwip/udp.h>

#include <stdlib.h>

typedef struct {
  struct udp_pcb *pcb;
  const DataRecvTask_t *recv_task;
} UdpServerWidgetState_t;

static void udp_recv_proc(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                          const ip_addr_t *addr, u16_t port) {
  UdpServerWidgetState_t *state = (UdpServerWidgetState_t *)arg;
  const DataRecvTask_t *recv_task = state->recv_task;
  recv_task->callback(recv_task->context, p);
  pbuf_free(p);
}

static UdpServerWidgetState_t state = {0};

static void *init_state(mContext_t *context) {
  assert(state.pcb == NULL);
  const DataRecvTask_t *const recv_task =
      (const DataRecvTask_t *)m_get_widget_data(context);
  state.recv_task = recv_task;

  udp_init();
  struct udp_pcb *pcb = udp_new();
  assert(pcb != NULL);
  err_t err = udp_bind(pcb, IP_ADDR_ANY, 5400);
  assert(err == ERR_OK);
  udp_recv(pcb, udp_recv_proc, &state);
  state.pcb = pcb;

  return &state;
}

static void build(mContext_t *context, void *_state,
                  const mWidget_t *children[MF_MAX_CHILDREN],
                  const void *children_widget_data[MF_MAX_CHILDREN]) {
  UdpServerWidgetState_t *state = (UdpServerWidgetState_t *)_state;
  const DataRecvTask_t *const recv_task =
      (const DataRecvTask_t *)m_get_widget_data(context);
  state->recv_task = recv_task;
  return;
}

static void dispose(mContext_t *context, void *_state) {
  UdpServerWidgetState_t *state = (UdpServerWidgetState_t *)_state;
  udp_remove(state->pcb);
  state->pcb = NULL;
  return;
};

WIDGET_MEMORY_LOCATION const mWidget_t UdpServerWidget = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
