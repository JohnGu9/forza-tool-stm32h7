#include "app.h"
#include "common.h"
#include "io.h"
#include "stm32h7xx_hal_def.h"
#include "widget/widgets.h"

#include "lwip/timeouts.h"
#include "tusb.h"

#include "mf.h"

#include "cmsis_os2.h"
#include "stm32h7xx_hal_conf.h"

#include <stdlib.h>
#include <string.h>

static mContext_t *new_context() {
  return (mContext_t *)malloc(sizeof(mContext_t));
}

static void delete_context(mContext_t *context) { free(context); }

static void on_widget_error(const char *reason) {
  UNUSED(reason);
  return;
}

void run_app(const AppContext_t *context) {
  io_init();

  mContext_t widget_context = {0};
  m_attach(&widget_context, &MainWidget, &new_context, &delete_context,
           &on_widget_error);

  uint32_t flags;
  EventTask_t task;
  osStatus_t status;

  status = osTimerStart(lvglTimerHandle, LV_DEF_REFR_PERIOD);
  assert_param(status == osOK);

  status = osTimerStart(lwipTimerHandle, 500);
  assert_param(status == osOK);

  status = osTimerStart(usbTimerHandle, 1);
  assert_param(status == osOK);

  lv_timer_handler();

  /* Infinite loop */
  for (;;) {
    flags = osEventFlagsWait(appEventHandle, APP_EVENT_ALL, osFlagsWaitAny,
                             osWaitForever);
    if (flags & osFlagsError) {
      // @TODO:
    } else {
      if (flags | APP_EVENT_USB) {
        tud_task();
        // ensure [tud_task] be called as max time gap 100ms
        osTimerStart(usbTimerHandle, 100);
      }
      if (flags | APP_EVENT_LWIP) {
        sys_check_timeouts();
      }
    }
    for (; osMessageQueueGetCount(defaultQueueHandle) != 0;) {
      flags =
          osEventFlagsWait(appEventHandle, APP_EVENT_ALL, osFlagsNoClear, 0);
      if ((flags & osFlagsError) == 0 && flags != 0) {
        break;
      }
      status = osMessageQueueGet(defaultQueueHandle, &task, NULL, 0);
      if (status == osOK) {
        task.callback(task.context);
      }
    }
  }
}

size_t board_usb_get_serial(uint16_t id[], size_t max_len) {
  UNUSED(max_len);
  uint32_t uuid[] = {
      HAL_GetUIDw0(),
      HAL_GetUIDw1(),
      HAL_GetUIDw2(),
  };
  assert_param(max_len > sizeof(uuid));
  memcpy(id, uuid, sizeof(uuid));
  return sizeof(uuid);
}

void schedule_task_on_main_thread(const EventTask_t *task) {
  osStatus_t status = osMessageQueuePut(defaultQueueHandle, task, 0, 0);
  if (status == osOK) {
    osEventFlagsSet(appEventHandle, APP_EVENT_NORMAL);
  }
}

void usbTimerCallback(void *argument) {
  osEventFlagsSet(appEventHandle, APP_EVENT_USB);
}

static void lv_timer_handler_async(void *context) {
  UNUSED(context);
  lv_timer_handler();
}

void lvglTimerCallback(void *argument) {
  const EventTask_t task = {
      .callback = &lv_timer_handler_async,
      .context = NULL,
  };
  schedule_task_on_main_thread(&task);
}

void lwipTimerCallback(void *argument) {
  osEventFlagsSet(appEventHandle, APP_EVENT_LWIP);
}
