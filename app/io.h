#ifndef IO_H
#define IO_H
#include "common.h"

#include <lvgl.h>
#include <stdbool.h>

#include "cmsis_os2.h"
#include "stm32h7xx_hal.h"

extern SPI_HandleTypeDef hspi2;
extern osMessageQueueId_t spi2TxQueueHandle;
extern osMessageQueueId_t spi2TxCompletedQueueHandle;
extern osEventFlagsId_t displayReadyEventHandle;

extern lv_display_t *display;

void init_io();
void deinit_io();

// only call in irq
void handle_otg_irq();

extern EventTaskSet_t k1_listeners;
extern EventTaskSet_t k2_listeners;

extern EventTaskSet_t link_state_listeners;
bool is_link_state_up();


#endif
