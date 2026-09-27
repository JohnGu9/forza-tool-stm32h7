#include "common.h"
#include "widgets.h"

#include "mf.h"

#include <lvgl.h>
#include <stdlib.h>

typedef struct {
  lv_obj_t *div;
  lv_obj_t *led_1;
  lv_obj_t *led_2;
  lv_obj_t *led_3;
  lv_obj_t *led_4;
  lv_obj_t *led_5;
  lv_timer_t *led_blink_timer;
  bool led_on;
} TachometerLedControllerWidgetState_t;

static uint8_t get_brightness(float progress, float min, float max) {
  if (progress <= min)
    return 0;
  if (progress >= max)
    return 0xFF;
  return (progress - min) / (max - min) * 0xFF;
}

static void led_blink_timer_callback(lv_timer_t *timer) {
  TachometerLedControllerWidgetState_t *const state =
      (TachometerLedControllerWidgetState_t *)lv_timer_get_user_data(timer);
  state->led_on =
      !state->led_on; // should call m_set_state but not necessary :)
}

static void reset_to_default(TachometerLedControllerWidgetState_t *state) {
  lv_led_set_brightness(state->led_1, 0);
  lv_led_set_brightness(state->led_2, 0);
  lv_led_set_brightness(state->led_3, 0);
  lv_led_set_brightness(state->led_4, 0);
  lv_led_set_brightness(state->led_5, 0);

  lv_led_set_color(state->led_1, lv_palette_main(LV_PALETTE_BLUE));
  lv_led_set_color(state->led_2, lv_palette_main(LV_PALETTE_BLUE));
  lv_led_set_color(state->led_4, lv_palette_main(LV_PALETTE_BLUE));
  lv_led_set_color(state->led_5, lv_palette_main(LV_PALETTE_BLUE));
}

static void set_led_light(TachometerLedControllerWidgetState_t *state,
                          const DataPacket_t *data_packet_nullable, // nullable
                          const DataAnalysis_t *data_analysis) {

  if (data_packet_nullable == NULL) {
    reset_to_default(state);
    return;
  }
  const DataPacket_t *const data_packet = data_packet_nullable;
  const SledData_t *const sled =
      get_sled_data(data_packet->buffer, data_packet->length);
  const DashData_t *const dash =
      get_dash_data(data_packet->buffer, data_packet->length);
  if (sled == NULL || dash == NULL) {
    reset_to_default(state);
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
    lv_led_set_brightness(state->led_3, get_brightness(progress, 0.333, 0.667));
    lv_led_set_brightness(state->led_4, get_brightness(progress, 0.667, 0.833));
    lv_led_set_brightness(state->led_5, get_brightness(progress, 0.833, 1));
  } else {
    lv_led_set_brightness(state->led_1, 0);
    lv_led_set_brightness(state->led_2, 0);
    lv_led_set_brightness(state->led_3, 0);
    lv_led_set_brightness(state->led_4, 0);
    lv_led_set_brightness(state->led_5, 0);
  }
  if (power_level > 0.99) {
    lv_led_set_color(state->led_1, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_set_color(state->led_2, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_set_color(state->led_4, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_set_color(state->led_5, lv_palette_main(LV_PALETTE_GREEN));
  } else {
    lv_led_set_color(state->led_1, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_set_color(state->led_2, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_set_color(state->led_4, lv_palette_main(LV_PALETTE_BLUE));
    lv_led_set_color(state->led_5, lv_palette_main(LV_PALETTE_BLUE));
  }
}

static void lv_led(TachometerLedControllerWidgetState_t *state,
                   lv_obj_t *parent) {
  lv_obj_t *div = lv_obj_create(parent);
  state->div = div;
  lv_obj_set_height(div, 36);
  lv_obj_set_flex_flow(div, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(div, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scrollable(div, false);

  lv_obj_t *led_1 = lv_led_create(div);
  state->led_1 = led_1;
  lv_obj_set_size(led_1, 10, 10);

  lv_obj_t *led_2 = lv_led_create(div);
  state->led_2 = led_2;
  lv_obj_set_size(led_2, 10, 10);

  lv_obj_t *led_3 = lv_led_create(div);
  state->led_3 = led_3;
  lv_obj_set_size(led_3, 10, 10);
  lv_led_set_color(led_3, lv_palette_main(LV_PALETTE_GREEN));

  lv_obj_t *led_4 = lv_led_create(div);
  state->led_4 = led_4;
  lv_obj_set_size(led_4, 10, 10);

  lv_obj_t *led_5 = lv_led_create(div);
  state->led_5 = led_5;
  lv_obj_set_size(led_5, 10, 10);
}

static void *init_state(mContext_t *context) {
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  TachometerLedControllerWidgetState_t *const state =
      new_object(TachometerLedControllerWidgetState_t);
  state->led_on = true;
  const DataPacket_t *data_packet =
      circular_buffer_get_last(data->circular_buffer);
  lv_led(state, data->div);
  set_led_light(state, data_packet, data->data_analysis);
  return state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  m_get_state_cast(state, context, TachometerLedControllerWidgetState_t);
  const DataPacket_t *data_packet =
      circular_buffer_get_last(data->circular_buffer);
  set_led_light(state, data_packet, data->data_analysis);
  return;
}

static void dispose(mContext_t *context) {
  m_get_state_cast(state, context, TachometerLedControllerWidgetState_t);

  if (state->led_blink_timer != NULL) {
    lv_timer_del(state->led_blink_timer);
    state->led_blink_timer = NULL;
  }
  lv_obj_del(state->div);
  delete_object(state);
  return;
};

const mWidgetClass_t TachometerLedControllerWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
