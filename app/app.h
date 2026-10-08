#ifndef APP_H
#define APP_H

#include "cmsis_os2.h"

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
} AppContext_t;

extern osMessageQueueId_t defaultQueueHandle;
extern osTimerId_t lvglTimerHandle;
extern osTimerId_t lwipTimerHandle;
extern osTimerId_t usbTimerHandle;
extern osTimerId_t k1TimerHandle;
extern osTimerId_t k2TimerHandle;
extern osTimerId_t iwdgRefreshTimerHandle;

extern osEventFlagsId_t appEventHandle;
#define APP_EVENT_NORMAL (1U << 0)
#define APP_EVENT_USB (1U << 1)
#define APP_EVENT_LVGL (1U << 2)
#define APP_EVENT_LWIP (1U << 3)
#define APP_EVENT_K1 (1U << 4)
#define APP_EVENT_K2 (1U << 5)
#define APP_EVENT_K1_CONFIRM (1U << 6)
#define APP_EVENT_K2_CONFIRM (1U << 7)
#define APP_EVENT_IWDG (1U << 8)

#define APP_EVENT_ALL  0xFFFFFF

void run_app(const AppContext_t *context);
void schedule_task_on_main_thread(const EventTask_t *task);

#ifdef __cplusplus
}
#endif

#endif
