#include "widgets.h"

#include "mf.h"

#include <lvgl.h>
#include <stdlib.h>

typedef struct {
} TachometerLedControllerWidgetState_t;

static uint8_t get_brightness(float progress, float min, float max) {
  if (progress <= min)
    return 0;
  if (progress >= max)
    return 0xFF;
  return (progress - min) / (max - min) * 0xFF;
}

static void led_blink_timer_callback(lv_timer_t *timer) {
  TachometerWidgetState_t *const state =
      (TachometerWidgetState_t *)lv_timer_get_user_data(timer);
  state->led_on =
      !state->led_on; // should call m_set_state but not necessary :)
}

static void set_led_light(TachometerWidgetState_t *state,
                          const DataPacket_t *data_packet_nullable, // nullable
                          const DataAnalysis_t *data_analysis) {

  if (data_packet_nullable == NULL) {
    return;
  }
  const DataPacket_t *const data_packet = data_packet_nullable;
  const SledData_t *const sled =
      get_sled_data(data_packet->buffer, data_packet->length);
  const DashData_t *const dash =
      get_dash_data(data_packet->buffer, data_packet->length);
  if (sled == NULL || dash == NULL) {
    return;
  }

  float power_level = dash->Power / data_analysis->max_power.Power;
  float progress;
  if (sled->CurrentEngineRpm < data_analysis->max_power.CurrentEngineRpm) {
    if (power_level < 0.97) {
      progress = 0;
    } else {
      progress = (1 - (1 - power_level) / 0.03) / 2;
    }
  } else {
    if (power_level < 0.97) {
      progress = 1.0000001;
    } else {
      progress = 0.5 + (1 - power_level) / 0.03 / 2;
    }
  }
  if (progress >= 1) {
    if (state->led_blink_timer == NULL) {
      state->led_blink_timer =
          lv_timer_create(led_blink_timer_callback, 200, state);
    }
  } else {
    if (state->led_blink_timer != NULL) {
      lv_timer_del(state->led_blink_timer);
      state->led_blink_timer = NULL;
      state->led_on = true;
    }
  }
  if (state->led_on) {
    lv_led_set_brightness(state->led_1, get_brightness(progress, 0, 0.167));
    lv_led_set_brightness(state->led_2, get_brightness(progress, 0.167, 0.333));
    lv_led_set_brightness(state->led_4, get_brightness(progress, 0.667, 0.833));
    lv_led_set_brightness(state->led_5, get_brightness(progress, 0.833, 1));
  } else {
    lv_led_set_brightness(state->led_1, 0);
    lv_led_set_brightness(state->led_2, 0);
    lv_led_set_brightness(state->led_4, 0);
    lv_led_set_brightness(state->led_5, 0);
  }
  lv_led_set_brightness(state->led_3, power_level > 0.99 ? 0xFF : 0);
}

static void *init_state(mContext_t *context) {
  TachometerWidgetState_t *const tachometer_widget_state =
      (TachometerWidgetState_t *)m_get_widget_data(context);
  tachometer_widget_state->led_on = true;
  const DataPacket_t *data_packet = circular_buffer_get_last(
      tachometer_widget_state->link_up_widget_state->circular_buffer);
  set_led_light(tachometer_widget_state, data_packet,
                tachometer_widget_state->link_up_widget_state->data_analysis);
  return NULL;
}

static void build(mContext_t *context, void *state,
                  const mWidget_t *children[MF_MAX_CHILDREN],
                  const void *children_widget_data[MF_MAX_CHILDREN]) {
  TachometerWidgetState_t *const tachometer_widget_state =
      (TachometerWidgetState_t *)m_get_widget_data(context);
  const DataPacket_t *data_packet = circular_buffer_get_last(
      tachometer_widget_state->link_up_widget_state->circular_buffer);
  set_led_light(tachometer_widget_state, data_packet,
                tachometer_widget_state->link_up_widget_state->data_analysis);
  return;
}

static void dispose(mContext_t *context, void *state) {
  TachometerWidgetState_t *const tachometer_widget_state =
      (TachometerWidgetState_t *)m_get_widget_data(context);
  if (tachometer_widget_state->led_blink_timer != NULL) {
    lv_timer_del(tachometer_widget_state->led_blink_timer);
    tachometer_widget_state->led_blink_timer = NULL;
  }
  return;
};

const mWidget_t TachometerLedControllerWidget = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
