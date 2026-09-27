#include "app.h"
#include "common.h"
#include "io.h"
#include "main.h"
#include "widget/widgets.h"

#include "mf.h"

#include "lwip/timeouts.h"
#include "tusb.h"

#include "cmsis_os2.h"
#include "stm32h7xx_hal_gpio.h"

#include <assert.h>
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
  init_io();

  mContext_t widget_context = {0};
  mWidget_t main_widget = {.class = &MainWidgetClass, .data = NULL};
  m_attach(&widget_context, &main_widget, &new_context, &delete_context,
           &on_widget_error);

  uint32_t flags;
  EventTask_t task;
  osStatus_t status;

  status = osTimerStart(lvglTimerHandle, LV_DEF_REFR_PERIOD);
  assert(status == osOK);

  status = osTimerStart(lwipTimerHandle, 500);
  assert(status == osOK);

  status = osTimerStart(usbTimerHandle, 1);
  assert(status == osOK);

  lv_timer_handler();

  /* Infinite loop */
  for (;;) {
    flags = osEventFlagsWait(appEventHandle, APP_EVENT_ALL, osFlagsWaitAny,
                             osWaitForever);
    if (unlikely(flags & osFlagsError)) {
      // @TODO:
    } else {
      if (flags & APP_EVENT_LVGL) {
        lv_timer_handler();
      }
      if (flags & APP_EVENT_USB) {
        tud_task();
        // ensure [tud_task] be called as max time gap 200ms
        osTimerStart(usbTimerHandle, 200);
      }
      if (flags & APP_EVENT_LWIP) {
        sys_check_timeouts();
      }
      if (flags & APP_EVENT_K1) {
        osTimerStart(k1TimerHandle, 100);
      }
      if (flags & APP_EVENT_K2) {
        osTimerStart(k2TimerHandle, 100);
      }
    }
    for (; osMessageQueueGetCount(defaultQueueHandle) != 0;) {
      flags =
          osEventFlagsWait(appEventHandle, APP_EVENT_ALL, osFlagsNoClear, 0);
      if ((flags & osFlagsError) == 0 && flags != 0) {
        break;
      }
      status = osMessageQueueGet(defaultQueueHandle, &task, NULL, 0);
      if (likely(status == osOK)) {
        task.callback(task.context);
      }
    }
  }

  m_detach(&widget_context);
  deinit_io();
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
  if (likely(status == osOK)) {
    osEventFlagsSet(appEventHandle, APP_EVENT_NORMAL);
  }
}

void usbTimerCallback(void *argument) {
  osEventFlagsSet(appEventHandle, APP_EVENT_USB);
}

void lvglTimerCallback(void *argument) {
  osEventFlagsSet(appEventHandle, APP_EVENT_LVGL);
}

void lwipTimerCallback(void *argument) {
  osEventFlagsSet(appEventHandle, APP_EVENT_LWIP);
}

static void k1_async(void *context) { invoke_tasks(&k1_listeners); }

void k1TimerCallback(void *argument) {
  GPIO_PinState state = HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin);
  if (state == GPIO_PIN_RESET) {
    const EventTask_t task = {
        .callback = &k1_async,
        .context = NULL,
    };
    schedule_task_on_main_thread(&task);
  }
}

static void k2_async(void *context) { invoke_tasks(&k2_listeners); }

void k2TimerCallback(void *argument) {
  GPIO_PinState state = HAL_GPIO_ReadPin(K2_GPIO_Port, K2_Pin);
  if (state == GPIO_PIN_RESET) {
    const EventTask_t task = {
        .callback = &k2_async,
        .context = NULL,
    };
    schedule_task_on_main_thread(&task);
  }
}
