#include "common.h"
#include "mf.h"
#include "widgets.h"
#include <lvgl.h>
#include <lvgl/core/lv_obj_pos.h>
#include <lvgl/draw/lv_color.h>
#include <stdlib.h>

typedef struct {
  lv_obj_t *chart;
  lv_obj_t *bar_steer;
  lv_chart_series_t *ser;
  EventTask_t data_recv_listener;
  mContext_t *context;
} GForceWidgetState_t;

static void draw_event_cb(lv_event_t *e) {
  lv_draw_task_t *draw_task = lv_event_get_draw_task(e);
  lv_draw_dsc_base_t *base_dsc =
      (lv_draw_dsc_base_t *)lv_draw_task_get_draw_dsc(draw_task);
  if (base_dsc->part != LV_PART_INDICATOR)
    return;
  lv_obj_t *obj = lv_event_get_target_obj(e);

  lv_draw_fill_dsc_t *fill_draw_dsc = lv_draw_task_get_fill_dsc(draw_task);
  if (fill_draw_dsc == NULL)
    return;

  uint32_t cnt = lv_chart_get_point_count(obj);

  /*Make older value more transparent*/

  fill_draw_dsc->opa = (lv_opa_t)((LV_OPA_COVER * base_dsc->id2) / (cnt - 1));

  fill_draw_dsc->color = lv_palette_main(LV_PALETTE_BLUE);
}

static void add_data(GForceWidgetState_t *const state,
                     const DataPacket_t *const data_packet) {

  const SledData_t *sled =
      get_sled_data(data_packet->buffer, data_packet->length);
  if (unlikely(sled == NULL)) {
    return;
  }
  lv_chart_set_next_value2(state->chart, state->ser,
                           -100 / 9.8067 * sled->AccelerationX,
                           -100 / 9.8067 * sled->AccelerationZ);
}

static void add_data_callback(void *ctx) {
  GForceWidgetState_t *const state = (GForceWidgetState_t *)ctx;
  mContext_t *const context = state->context;
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  const DataPacket_t *data_packet =
      circular_buffer_get_last(data->circular_buffer);
  if (unlikely(data_packet == NULL)) {
    return;
  }
  add_data(state, data_packet);
}

static void set_data(GForceWidgetState_t *const state,
                     const LinkUpWidgetState_t *const data) {
  int32_t steer = 0;
  const DataPacket_t *data_packet =
      circular_buffer_get_last(data->circular_buffer);
  if (likely(data_packet != NULL)) {
    const DashData_t *dash =
        get_dash_data(data_packet->buffer, data_packet->length);
    if (likely(dash != NULL)) {
      steer = dash->Steer;
    }
  }

  lv_bar_set_start_value(state->bar_steer, steer - 16, false);
  lv_bar_set_value(state->bar_steer, steer + 16, false);
  return;
}

static void *init_state(mContext_t *context) {
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  GForceWidgetState_t *const state = new_object(GForceWidgetState_t);

  const int32_t height = lv_obj_get_height(data->div);
  const int32_t width = lv_obj_get_width(data->div);
  const int32_t min_size = MIN(height, width);

  lv_obj_t *chart = lv_chart_create(data->div);
  state->chart = chart;
  lv_obj_set_size(chart, min_size, min_size);
  lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(chart, draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
  lv_obj_set_send_draw_task_events(chart, true);
  lv_obj_set_style_line_width(chart, 0, LV_PART_ITEMS); /*Remove the lines*/

  lv_chart_set_type(chart, LV_CHART_TYPE_SCATTER);

  lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_X, -300, 300);
  lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, -300, 300);

  lv_chart_set_point_count(chart, 50);

  lv_chart_series_t *ser = lv_chart_add_series(
      chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  state->ser = ser;
  for (size_t i = 0; i < data->circular_buffer->length; i++) {
    add_data(state, circular_buffer_get(data->circular_buffer, i));
  }

  lv_obj_t *bar_steer = lv_bar_create(data->div);
  state->bar_steer = bar_steer;
  lv_obj_set_size(bar_steer, lv_pct(90), 12);
  lv_bar_set_mode(bar_steer, LV_BAR_MODE_RANGE);
  lv_bar_set_min_value(bar_steer, -128);
  lv_bar_set_max_value(bar_steer, 128);
  lv_obj_t *label_steer = lv_label_create(bar_steer);
  lv_obj_set_align(label_steer, LV_ALIGN_CENTER);
  lv_label_set_text(label_steer, "Steer");
  set_data(state, data);

  state->context = context;
  state->data_recv_listener = (EventTask_t){
      .context = state,
      .callback = &add_data_callback,
  };
  add_task((EventTaskSet_t *)&data->data_recv_listeners,
           &state->data_recv_listener);

  return state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  m_get_state_cast(state, context, GForceWidgetState_t);
  set_data(state, data);
  return;
}

static void dispose(mContext_t *context) {
  m_get_widget_data_cast(data, context, LinkUpWidgetState_t);
  m_get_state_cast(state, context, GForceWidgetState_t);
  remove_task((EventTaskSet_t *)&data->data_recv_listeners,
              &state->data_recv_listener);
  lv_obj_del(state->bar_steer);
  lv_obj_del(state->chart);
  delete_object(state);
  return;
};

const mWidgetClass_t GForceWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
