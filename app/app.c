#include "app.h"
#include "common.h"
#include "io.h"
#include "main.h"
#include "widget/widgets.h"

#include "mf.h"

#include "lwip/timeouts.h"
#include "tusb.h"

#include "cmsis_os2.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

extern IWDG_HandleTypeDef hiwdg1;

static mContext_t *new_context(void *ctx) {
  osMemoryPoolId_t context_mem_pool = (osMemoryPoolId_t)ctx;
  return (mContext_t *)osMemoryPoolAlloc(context_mem_pool, 0U);
}

static void delete_context(void *ctx, mContext_t *context) {
  osMemoryPoolId_t context_mem_pool = (osMemoryPoolId_t)ctx;
  osMemoryPoolFree(context_mem_pool, context);
}

static void on_widget_error(const char *reason) {
  UNUSED(reason);
  return;
}

void run_app(const AppContext_t *context) {
  init_io();

  osMemoryPoolId_t context_mem_pool =
      osMemoryPoolNew(64, sizeof(mContext_t), NULL);
  assert(context_mem_pool != NULL);

  mContext_t *widget_context = osMemoryPoolAlloc(context_mem_pool, 0U);
  assert(widget_context != NULL);
  memset(widget_context, 0, sizeof(*widget_context));

  const mWidget_t main_widget = {.class = &MainWidgetClass, .data = NULL};
  m_attach(widget_context, &main_widget, context_mem_pool, &new_context,
           &delete_context, &on_widget_error);

  uint32_t flags;
  EventTask_t task;
  osStatus_t status;

  status = osTimerStart(iwdgRefreshTimerHandle, 1000);
  assert(status == osOK);

  status = osTimerStart(lvglTimerHandle, LV_DEF_REFR_PERIOD);
  assert(status == osOK);

  status = osTimerStart(lwipTimerHandle, 500);
  assert(status == osOK);

  status = osTimerStart(usbTimerHandle, 5);
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
      if (flags & APP_EVENT_K1_CONFIRM) {
        invoke_tasks(&k1_listeners);
      }
      if (flags & APP_EVENT_K2_CONFIRM) {
        invoke_tasks(&k2_listeners);
      }
      if (flags & APP_EVENT_IWDG) {
        HAL_IWDG_Refresh(&hiwdg1);
      }
    }
    for (; osMessageQueueGetCount(defaultQueueHandle) != 0;) {
      status = osMessageQueueGet(defaultQueueHandle, &task, NULL, 0);
      if (likely(status == osOK)) {
        task.callback(task.context);
      }
      flags =
          osEventFlagsWait(appEventHandle, APP_EVENT_ALL, osFlagsNoClear, 0);
      if ((flags & osFlagsError) == 0 && flags != 0) {
        break;
      }
    }
  }

  m_detach(widget_context);
  osMemoryPoolFree(context_mem_pool, widget_context);
  osMemoryPoolDelete(context_mem_pool);

  deinit_io();
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

void k1TimerCallback(void *argument) {
  const GPIO_PinState state = HAL_GPIO_ReadPin(K1_GPIO_Port, K1_Pin);
  if (state == GPIO_PIN_RESET) {
    osEventFlagsSet(appEventHandle, APP_EVENT_K1_CONFIRM);
  }
}

void k2TimerCallback(void *argument) {
  const GPIO_PinState state = HAL_GPIO_ReadPin(K2_GPIO_Port, K2_Pin);
  if (state == GPIO_PIN_RESET) {
    osEventFlagsSet(appEventHandle, APP_EVENT_K2_CONFIRM);
  }
}

void iwdgRefreshTimerCallback(void *argument) {
  osEventFlagsSet(appEventHandle, APP_EVENT_IWDG);
}
