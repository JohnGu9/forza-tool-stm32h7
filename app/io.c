#include "io.h"
#include "app.h"
#include "common.h"
#include "common/tusb_common.h"
#include "main.h"

#include "st7735.h"

#include "cmsis_os2.h"
#include "stm32h7xx_hal_def.h"
#include "stm32h7xx_hal_spi.h"

#include <malloc.h>

// display

lv_display_t *display;

static St7735Handle_t st7735;

static int32_t st7735_delay(St7735Handle_t *self, uint32_t ms) {
  UNUSED(self);
  return osDelay(ms);
}

static int32_t st7735_set_chip_select(St7735Handle_t *self,
                                      St7735PinState state) {
  UNUSED(self);
  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, (GPIO_PinState)state);
  return HAL_OK;
}

static int32_t st7735_set_reset_or_dc(St7735Handle_t *self,
                                      St7735PinState state) {
  UNUSED(self);
  HAL_GPIO_WritePin(DC_GPIO_Port, DC_Pin, (GPIO_PinState)state);
  return HAL_OK;
}

static uint32_t unused_value = 0;

static int32_t st7735_transmit(St7735Handle_t *self, const uint8_t *pData,
                               size_t size) {
  UNUSED(self);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 0);
  HAL_StatusTypeDef ret = HAL_OK;
  for (; size != 0;) {
    const uint16_t transmit_size = MIN(size, 0xFFFFU);
    HAL_StatusTypeDef status =
        HAL_SPI_Transmit_DMA(&hspi2, pData, transmit_size);
    if (status == HAL_OK) {
      // wait for [HAL_SPI_TxCpltCallback] | [HAL_SPI_ErrorCallback]
      osMessageQueueGet(spi2TxCompletedQueueHandle, &unused_value, NULL,
                        osWaitForever);
    }
    ret |= status;
    size -= transmit_size;
    pData += transmit_size;
  }

  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, 1);
  return ret;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi2) { // likely
    osMessageQueuePut(spi2TxCompletedQueueHandle, &unused_value, 0, 0);
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
  if (hspi == &hspi2) { // likely
    osMessageQueuePut(spi2TxCompletedQueueHandle, &unused_value, 0, 0);
  }
}

typedef struct {
  lv_display_t *disp;
  lv_area_t area;
  uint8_t *px_buf;
} LvglFlushAsyncContext_t;

static void lvgl_flush_async(void *context) {
  LvglFlushAsyncContext_t *ctx = (LvglFlushAsyncContext_t *)context;
  ST7735_DrawImage(
      &st7735, ctx->area.x1, ctx->area.y1, ctx->area.x2 - ctx->area.x1 + 1,
      ctx->area.y2 - ctx->area.y1 + 1, (const uint16_t *)ctx->px_buf);
  lv_display_flush_ready(ctx->disp);
}

static void lvgl_flush(lv_display_t *disp, const lv_area_t *area,
                       uint8_t *px_buf) {
  static LvglFlushAsyncContext_t ctx;
  ctx.disp = disp;
  ctx.area.x1 = area->x1;
  ctx.area.x2 = area->x2;
  ctx.area.y1 = area->y1;
  ctx.area.y2 = area->y2;
  ctx.px_buf = px_buf;
  EventTask_t task = {
      .context = &ctx,
      .callback = lvgl_flush_async,
  };
  osStatus_t status = osMessageQueuePut(spi2TxQueueHandle, &task, 0, 0);
  if (status != osOK) { // give up flush
    lv_display_flush_ready(disp);
  }
}

static uint8_t lvgl_buffer[ST7735_WIDTH * ST7735_HEIGHT * 2]
    __attribute__((section(".RAM_D2")));
static uint8_t lvgl_buffer_second[ST7735_WIDTH * ST7735_HEIGHT * 2]
    __attribute__((section(".RAM_D2")));

void StartSpi2TxTask(void *argument) {
  // init display
  st7735.context = NULL;
  st7735.transmit = &st7735_transmit;
  st7735.delay = &st7735_delay;
  st7735.setChipSelect = &st7735_set_chip_select;
  st7735.setDc = &st7735_set_reset_or_dc;
  st7735.setReset = &st7735_set_reset_or_dc;
  ST7735_Init(&st7735);
  ST7735_FillScreenFast(&st7735, ST7735_WHITE);

  lv_init();

  lv_tick_set_cb(HAL_GetTick);

  display = lv_display_create(ST7735_WIDTH, ST7735_HEIGHT);
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565_SWAPPED);
  lv_display_set_buffers(display, lvgl_buffer, lvgl_buffer_second,
                         sizeof(lvgl_buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, lvgl_flush);

  osEventFlagsSet(displayReadyEventHandle, 1U);

  EventTask_t task;
  osStatus_t status;

  /* Infinite loop */
  for (;;) {
    status = osMessageQueueGet(spi2TxQueueHandle, &task, NULL, osWaitForever);
    if (status == osOK) {
      task.callback(task.context);
    }
  }
}

// usb network card

#include "dhserver.h"
#include "dnserver.h"
#include "lwip/ethip6.h"
#include "lwip/init.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "tusb.h"

#define INIT_IP4(a, b, c, d) {PP_HTONL(LWIP_MAKEU32(a, b, c, d))}

/* lwip context */
static struct netif netif_data = {0};

/* this is used by this code, ./class/net/net_driver.c, and usb_descriptors.c */
/* ideally speaking, this should be generated from the hardware's unique ID (if
 * available) */
/* it is suggested that the first byte is 0x02 to indicate a link-local address
 */
uint8_t tud_network_mac_address[6] = {0x02, 0x02, 0x84, 0x6A, 0x96, 0x00};

/* network parameters of this MCU */
static const ip4_addr_t ipaddr = INIT_IP4(192, 168, 7, 1);
static const ip4_addr_t netmask = INIT_IP4(255, 255, 255, 0);
static const ip4_addr_t gateway = INIT_IP4(0, 0, 0, 0);

/* database IP addresses that can be offered to the host; this must be in RAM to
 * store assigned MAC addresses */
static dhcp_entry_t entries[] = {
    /* mac ip address               lease time */
    {{0}, INIT_IP4(192, 168, 7, 2), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 3), 24 * 60 * 60},
    {{0}, INIT_IP4(192, 168, 7, 4), 24 * 60 * 60},
};

static const dhcp_config_t dhcp_config = {
    .router = INIT_IP4(0, 0, 0, 0),  /* router address (if any) */
    .port = 67,                      /* listen port */
    .dns = INIT_IP4(192, 168, 7, 1), /* dns server (if any) */
    "usb",                           /* dns suffix */
    TU_ARRAY_SIZE(entries),          /* num entry */
    entries                          /* entries */
};

static err_t linkoutput_fn(struct netif *netif, struct pbuf *p) {
  (void)netif;

  for (;;) {
    /* if TinyUSB isn't ready, we must signal back to lwip that there is nothing
     * we can do */
    if (!tud_ready())
      return ERR_USE;

    /* if the network driver can accept another packet, we make it happen */
    if (tud_network_can_xmit(p->tot_len)) {
      tud_network_xmit(p, 0 /* unused for this example */);
      return ERR_OK;
    }

    /* transfer execution to TinyUSB in the hopes that it will finish
     * transmitting the prior packet */
    tud_task();
  }
}

static err_t ip4_output_fn(struct netif *netif, struct pbuf *p,
                           const ip4_addr_t *addr) {
  return etharp_output(netif, p, addr);
}

#if LWIP_IPV6
static err_t ip6_output_fn(struct netif *netif, struct pbuf *p,
                           const ip6_addr_t *addr) {
  return ethip6_output(netif, p, addr);
}
#endif

static err_t netif_init_cb(struct netif *netif) {
  LWIP_ASSERT("netif != NULL", (netif != NULL));
  netif->mtu = CFG_TUD_NET_MTU;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP |
                 NETIF_FLAG_UP;
  netif->state = NULL;
  netif->name[0] = 'E';
  netif->name[1] = 'X';
  netif->linkoutput = linkoutput_fn;
  netif->output = ip4_output_fn;
#if LWIP_IPV6
  netif->output_ip6 = ip6_output_fn;
#endif
  return ERR_OK;
}

/* notifies the USB host about the link state change. */
static void usbnet_netif_link_callback(struct netif *netif) {
  bool link_up = netif_is_link_up(netif);
  tud_network_link_state(BOARD_TUD_RHPORT, link_up);
}

static void init_lwip(void) {
  struct netif *netif = &netif_data;

  lwip_init();

  /* the lwip virtual MAC address must be different from the host's; to ensure
   * this, we toggle the LSbit */
  netif->hwaddr_len = sizeof(tud_network_mac_address);
  memcpy(netif->hwaddr, tud_network_mac_address,
         sizeof(tud_network_mac_address));
  netif->hwaddr[5] ^= 0x01;

  netif = netif_add(netif, &ipaddr, &netmask, &gateway, NULL, netif_init_cb,
                    ethernet_input);
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(netif, 1);
#endif
  netif_set_default(netif);

#if LWIP_NETIF_LINK_CALLBACK
  // Set the link callback to notify USB host about link state changes
  netif_set_link_callback(netif, usbnet_netif_link_callback);
  netif_set_link_up(netif);
#else
  tud_network_link_state(BOARD_TUD_RHPORT, true);
#endif
}

/* handle any DNS requests from dns-server */
static bool dns_query_proc(const char *name, ip4_addr_t *addr) {
  if (0 == strcmp(name, "tiny.usb")) {
    *addr = ipaddr;
    return true;
  }
  return false;
}

bool tud_network_recv_cb(const uint8_t *src, uint16_t size) {
  struct netif *netif = &netif_data;

  if (size) {
    struct pbuf *p = pbuf_alloc(PBUF_RAW, size, PBUF_POOL);

    if (p == NULL) {
      printf("ERROR: Failed to allocate pbuf of size %d\n", size);
      return false;
    }

    /* Copy buf to pbuf */
    pbuf_take(p, src, size);

    // Surrender ownership of our pbuf unless there was an error
    // Only call pbuf_free if not Ok else it will panic with "pbuf_free: p->ref
    // > 0" or steal it from whatever took ownership of it with undefined
    // consequences. See: https://savannah.nongnu.org/patch/index.php?10121
    if (netif->input(p, netif) != ERR_OK) {
      printf("ERROR: netif input failed\n");
      pbuf_free(p);
    }
    // Signal tinyusb that the current frame has been processed.
    tud_network_recv_renew();
  }

  return true;
}

uint16_t tud_network_xmit_cb(uint8_t *dst, void *ref, uint16_t arg) {
  struct pbuf *p = (struct pbuf *)ref;

  (void)arg; /* unused for this example */

  return pbuf_copy_partial(p, dst, p->tot_len, 0);
}

sys_prot_t sys_arch_protect(void) { return 0; }

void sys_arch_unprotect(sys_prot_t pval) { (void)pval; }

uint32_t sys_now(void) { return HAL_GetTick(); }

static bool is_link_up = false;

static EventTask_t *link_state_listeners[8];

int32_t add_link_state_listener(EventTask_t *task) {
  return add_task(task, link_state_listeners,
                  TU_ARRAY_SIZE(link_state_listeners));
}

int32_t remove_link_state_listener(EventTask_t *task) {
  return remove_task(task, link_state_listeners,
                     TU_ARRAY_SIZE(link_state_listeners));
}

bool is_link_state_up() { return is_link_up; }

// Invoked when device is mounted
void tud_mount_cb(void) {
  is_link_up = true;
  invoke_tasks(link_state_listeners, TU_ARRAY_SIZE(link_state_listeners));
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
  is_link_up = false;
  invoke_tasks(link_state_listeners, TU_ARRAY_SIZE(link_state_listeners));
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) {
  (void)remote_wakeup_en;
  is_link_up = false;
  invoke_tasks(link_state_listeners, TU_ARRAY_SIZE(link_state_listeners));
}

// Invoked when usb bus is resumed
void tud_resume_cb(void) {
  is_link_up = true;
  invoke_tasks(link_state_listeners, TU_ARRAY_SIZE(link_state_listeners));
}

void handle_otg_irq() {
  tusb_int_handler(BOARD_TUD_RHPORT, true);
  osEventFlagsSet(appEventHandle, APP_EVENT_USB);
}

void io_init() {
  /* initialize TinyUSB */

  // init device stack on configured roothub port
  tusb_rhport_init_t dev_init = {.role = TUSB_ROLE_DEVICE,
                                 .speed = TUSB_SPEED_AUTO};
  tusb_init(BOARD_TUD_RHPORT, &dev_init);

  /* initialize lwip, dhcp-server, dns-server, and http */
  init_lwip();
  while (!netif_is_up(&netif_data))
    ;
  while (dhserv_init(&dhcp_config) != ERR_OK)
    ;
  while (dnserv_init(IP_ADDR_ANY, 53, dns_query_proc) != ERR_OK)
    ;

  // spi2TxTask handle the display init task, just wait for the completed event
  osEventFlagsWait(displayReadyEventHandle, 1U, osFlagsWaitAny, osWaitForever);
  osEventFlagsDelete(displayReadyEventHandle);
}
