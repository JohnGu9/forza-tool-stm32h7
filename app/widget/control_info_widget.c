#include "common.h"
#include "mf.h"
#include "widgets.h"
#include <assert.h>
#include <lvgl.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  lv_obj_t *bar_accel;
  lv_obj_t *bar_brake;
  lv_obj_t *bar_power_level;
  lv_obj_t *label_power_level;
} ControlInfoWidgetState_t;

static void reset_data(ControlInfoWidgetState_t *const state) {
  lv_bar_set_value(state->bar_accel, 0, false);
  lv_bar_set_value(state->bar_brake, 0, false);
  lv_bar_set_value(state->bar_power_level, 0, false);
  lv_label_set_text(state->label_power_level, "Power: 0%");
}

static void set_data(ControlInfoWidgetState_t *const state,
                     const LinkUpWidgetState_t *const data) {
  const DataPacket_t *data_packet =
      circular_buffer_get_last(data->circular_buffer);
  if (data_packet == NULL) {
    reset_data(state);
    return;
  }
  const DashData_t *dash =
      get_dash_data(data_packet->buffer, data_packet->length);
  if (dash == NULL) {
    reset_data(state);
    return;
  }
  uint8_t temp;
  memcpy(&temp, &dash->Accel, sizeof(temp));
  lv_bar_set_value(state->bar_accel, temp, false);
  memcpy(&temp, &dash->Brake, sizeof(temp));
  lv_bar_set_value(state->bar_brake, temp, false);

  float max_power = data->data_analysis->max_power.Power;
  float power_level = max_power == 0 ? 0 : dash->Power / max_power;

  lv_bar_set_value(state->bar_power_level, 0xFF * power_level, false);
  lv_label_set_text_fmt(state->label_power_level, "Power: %d%%",
                        (int)(power_level * 100));
}

static void *init_state(mContext_t *context) {
  ControlInfoWidgetState_t *const state = new_object(ControlInfoWidgetState_t);
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  m_get_state_from_inherited_widget_class_cast(
      inherited_widget_state, context, &LinkUpWidgetClass, LinkUpWidgetState_t);
  assert(inherited_widget_state == data);

  lv_obj_t *bar_accel = lv_bar_create(data->div);
  state->bar_accel = bar_accel;
  lv_obj_set_size(bar_accel, lv_pct(90), 20);
  lv_bar_set_min_value(bar_accel, 0);
  lv_bar_set_max_value(bar_accel, 255);
  lv_obj_t *label_accel = lv_label_create(bar_accel);
  lv_obj_set_align(label_accel, LV_ALIGN_CENTER);
  lv_label_set_text(label_accel, "Accel");

  lv_obj_t *bar_brake = lv_bar_create(data->div);
  state->bar_brake = bar_brake;
  lv_obj_set_size(bar_brake, lv_pct(90), 20);
  lv_bar_set_min_value(bar_brake, 0);
  lv_bar_set_max_value(bar_brake, 255);
  lv_obj_t *label_brake = lv_label_create(bar_brake);
  lv_obj_set_align(label_brake, LV_ALIGN_CENTER);
  lv_label_set_text(label_brake, "Brake");

  lv_obj_t *bar_power_level = lv_bar_create(data->div);
  state->bar_power_level = bar_power_level;
  lv_obj_set_size(bar_power_level, lv_pct(90), 20);
  lv_bar_set_min_value(bar_power_level, 0);
  lv_bar_set_max_value(bar_power_level, 255);
  lv_obj_t *label_power_level = lv_label_create(bar_power_level);
  state->label_power_level = label_power_level;
  lv_obj_set_align(label_power_level, LV_ALIGN_CENTER);

  set_data(state, data);
  return state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  m_get_state_cast(state, context, ControlInfoWidgetState_t);
  set_data(state, data);
  return;
}

static void dispose(mContext_t *context) {
  m_get_state_cast(state, context, ControlInfoWidgetState_t);
  lv_obj_del(state->bar_power_level);
  lv_obj_del(state->bar_brake);
  lv_obj_del(state->bar_accel);
  delete_object(state);
  return;
};

const mWidgetClass_t ControlInfoWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
