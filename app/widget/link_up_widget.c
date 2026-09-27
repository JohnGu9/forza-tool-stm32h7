#include "animated_transition_widget.h"
#include "app.h"
#include "common.h"
#include "io.h"
#include "widgets.h"

#include "mf.h"

#include <assert.h>
#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static void div_anim_callback(lv_anim_t *var, int32_t v) {
  lv_obj_t *const div = (lv_obj_t *)var->var;
  lv_obj_set_y(div, v);
}

static CircularBuffer_t circular_buffer
    __attribute__((section(".DTCMRAM"))) = {0};

static DataAnalysis_t data_analysis __attribute__((section(".DTCMRAM"))) = {0};

static LinkUpWidgetState_t state = {0};

static void get_power_range(DataAnalysis_t *data_analysis,
                            const SledData_t *const sled) {
#define IS_VALUE_IN_RANGE(x, min, max) ((x) >= (min)) && ((x) <= (max))

  if (!IS_VALUE_IN_RANGE(data_analysis->max_power.CurrentEngineRpm,
                         sled->EngineIdleRpm, sled->EngineMaxRpm)) {
    return;
  }

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

  float chunk_size = (sled->EngineMaxRpm - sled->EngineIdleRpm) /
                     ARRAY_LENGTH(data_analysis->power_curve);
  size_t max_index_lower = (size_t)((data_analysis->max_power.CurrentEngineRpm -
                                     sled->EngineIdleRpm) /
                                    chunk_size);
  data_analysis->power_range.below_90_lower = NULL;
  data_analysis->power_range.below_97_lower = NULL;

  size_t i;
  for (i = max_index_lower; i != 0; i--) {
    const DataAnalysisMetaData_t *meta_data = &data_analysis->power_curve[i];
    if (!IS_VALUE_IN_RANGE(meta_data->CurrentEngineRpm, sled->EngineIdleRpm,
                           sled->EngineMaxRpm))
      continue;
    if (meta_data->Power < data_analysis->max_power.Power * 0.97) {
      data_analysis->power_range.below_97_lower = meta_data;
      break;
    }
  }
  for (; i != 0; i--) {
    const DataAnalysisMetaData_t *meta_data = &data_analysis->power_curve[i];
    if (!IS_VALUE_IN_RANGE(meta_data->CurrentEngineRpm, sled->EngineIdleRpm,
                           sled->EngineMaxRpm))
      continue;
    if (meta_data->Power < data_analysis->max_power.Power * 0.90) {
      data_analysis->power_range.below_90_lower = meta_data;
      break;
    }
  }

  data_analysis->power_range.below_90_upper = NULL;
  data_analysis->power_range.below_97_upper = NULL;

  for (i = max_index_lower + 1; i < ARRAY_LENGTH(data_analysis->power_curve);
       i++) {
    const DataAnalysisMetaData_t *meta_data = &data_analysis->power_curve[i];
    if (!IS_VALUE_IN_RANGE(meta_data->CurrentEngineRpm, sled->EngineIdleRpm,
                           sled->EngineMaxRpm))
      continue;
    if (meta_data->Power < data_analysis->max_power.Power * 0.97) {
      data_analysis->power_range.below_97_upper = meta_data;
      break;
    }
  }
  for (i++; i < ARRAY_LENGTH(data_analysis->power_curve); i++) {
    const DataAnalysisMetaData_t *meta_data = &data_analysis->power_curve[i];
    if (!IS_VALUE_IN_RANGE(meta_data->CurrentEngineRpm, sled->EngineIdleRpm,
                           sled->EngineMaxRpm))
      continue;
    if (meta_data->Power < data_analysis->max_power.Power * 0.90) {
      data_analysis->power_range.below_90_upper = meta_data;
      break;
    }
  }
}

static bool is_data_type_different(const DataPacket_t *const first,
                                   const struct pbuf *p) {
  if (first->length != p->len) {
    return true;
  } else {
    const SledData_t *new_data = (const SledData_t *)p->payload;
    const SledData_t *old_data = (const SledData_t *)first->buffer;
    if (new_data->CarOrdinal != old_data->CarOrdinal) {
      return true;
    }
  }
  return false;
}

static bool is_new_data_more_superior(const SledData_t *const sled,
                                      const DashData_t *dash,
                                      DataAnalysisMetaData_t *origin_meta_data,
                                      float target_rpm) {
  if (dash->Power < origin_meta_data->Power / 3 * 2) {
    return false;
  }
  float rpm_delta =
      abs(origin_meta_data->CurrentEngineRpm - sled->CurrentEngineRpm);
  if (rpm_delta < 50) {
    return dash->Power > origin_meta_data->Power;
  }

  int origin_rpm_delta = abs(origin_meta_data->CurrentEngineRpm - target_rpm);
  int new_rpm_delta = abs(sled->CurrentEngineRpm - target_rpm);
  return new_rpm_delta < origin_rpm_delta;
}

static bool fill_data(DataAnalysisMetaData_t *list, size_t length,
                      float rpm_min, float rpm_max,
                      const SledData_t *const sled, const DashData_t *dash) {
  float rpm_chunk_length = (rpm_max - rpm_min) / length;
  float rpm_chunk_index = (sled->CurrentEngineRpm - rpm_min) / rpm_chunk_length;
  size_t index = rpm_chunk_index;
  float mod = rpm_chunk_index - index;
  if (mod < 0.5) {
    DataAnalysisMetaData_t *origin_meta_data = &list[index];
    float target_rpm = rpm_min + index * rpm_chunk_length;
    if (is_new_data_more_superior(sled, dash, origin_meta_data, target_rpm)) {
      origin_meta_data->CurrentEngineRpm = sled->CurrentEngineRpm;
      origin_meta_data->Power = dash->Power;
      origin_meta_data->Torque = dash->Torque;
      return true;
    }
  } else {
    index = index + 1;
    if (index != length) {
      DataAnalysisMetaData_t *origin_meta_data = &list[index];
      float target_rpm = rpm_min + index * rpm_chunk_length;
      if (is_new_data_more_superior(sled, dash, origin_meta_data, target_rpm)) {
        origin_meta_data->CurrentEngineRpm = sled->CurrentEngineRpm;
        origin_meta_data->Power = dash->Power;
        origin_meta_data->Torque = dash->Torque;
        return true;
      }
    }
  }
  return false;
}

static void analysis_data(DataAnalysis_t *data_analysis,
                          const CircularBuffer_t *buffer) {
  const DataPacket_t *const last_data0 =
      circular_buffer_get(buffer, buffer->length - 1);
  const SledData_t *const sled0 =
      get_sled_data(last_data0->buffer, last_data0->length);
  const DashData_t *const dash0 =
      get_dash_data(last_data0->buffer, last_data0->length);
  if (sled0 == NULL || dash0 == NULL || dash0->Accel != 0xFF) {
    return;
  }
  if (buffer->length > 1) { // noise filter
    const DataPacket_t *const last_data1 =
        circular_buffer_get(buffer, buffer->length - 2);
    const SledData_t *const sled1 =
        get_sled_data(last_data1->buffer, last_data1->length);

    float delta_rpm = sled0->CurrentEngineRpm - sled1->CurrentEngineRpm;
    delta_rpm = delta_rpm < 0 ? -delta_rpm : delta_rpm; // abs
    if (delta_rpm > ((sled0->EngineMaxRpm - sled0->EngineIdleRpm) / 32)) {
      return;
    }
    const DashData_t *const dash1 =
        get_dash_data(last_data1->buffer, last_data1->length);
    if (dash0->Accel == 0xFF && dash1->Accel == 0xFF) {
      bool should_update_power_range = false;
      if (dash0->Power > data_analysis->max_power.Power) {
        data_analysis->max_power.Power = dash0->Power;
        data_analysis->max_power.CurrentEngineRpm = sled0->CurrentEngineRpm;
        data_analysis->max_power.Torque = dash0->Torque;
        should_update_power_range |= true;
      }
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))
      if (sled0->CurrentEngineRpm <= sled0->EngineMaxRpm ||
          sled0->CurrentEngineRpm >= sled0->EngineIdleRpm) {
        should_update_power_range |=
            fill_data(data_analysis->power_curve,
                      ARRAY_LENGTH(data_analysis->power_curve),
                      sled0->EngineIdleRpm, sled0->EngineMaxRpm, sled0, dash0);
      }

      if (should_update_power_range) {
        get_power_range(data_analysis, sled0);
      }
    }
  }
}

static void clear_data(LinkUpWidgetState_t *const state) {
  circular_buffer_clear(state->circular_buffer);
  data_analysis_clear(state->data_analysis);
}

static void set_state_async(void *context) {
  LinkUpWidgetState_t *const state = (LinkUpWidgetState_t *)context;
  if (state->is_dirty) {
    m_set_state(state->context);
    state->is_dirty = false;
  }
}

static void data_recv_callback(void *context, struct pbuf *p) {
  LinkUpWidgetState_t *const state = (LinkUpWidgetState_t *)context;
  switch (p->len) {
  case 323:
  case 324:

  case 311:
  case 312:

  case 331:
  case 332: {
    const SledData_t *data = (const SledData_t *)p->payload;
    if (data->IsRaceOn == 0) {
      break;
    }
    if (state->circular_buffer->length != 0) { // likely
      const DataPacket_t *const first =
          circular_buffer_get(state->circular_buffer, 0);
      if (is_data_type_different(first, p)) { // unlikely
        clear_data(state);
      }
    }
    DataPacket_t *data_packet = circular_buffer_push(state->circular_buffer);
    memcpy(data_packet->buffer, p->payload, p->len);
    data_packet->length = p->len;
    analysis_data(state->data_analysis, state->circular_buffer);
    invoke_tasks(&state->data_recv_listeners);
    state->is_dirty = true;
    const EventTask_t task = {
        .callback = &set_state_async,
        .context = state,
    };
    schedule_task_on_main_thread(&task);
    break;
  }
  }
}

static void on_k2(void *context) {
  LinkUpWidgetState_t *state = (LinkUpWidgetState_t *)context;
  clear_data(state);
  state->refresh_key++;
  m_set_state(state->context);
}

typedef enum {
  ChildType_Tachometer,
  ChildType_ControlInfo,
  ChildType_GForce,
} ChildType_t;

static ChildType_t get_next_key2(ChildType_t current) {
  switch (current) {
  case ChildType_Tachometer:
    return ChildType_ControlInfo;
  case ChildType_ControlInfo:
    return ChildType_GForce;
  case ChildType_GForce:
  default:
    return ChildType_Tachometer;
  }
}

static void on_k1(void *context) {
  LinkUpWidgetState_t *state = (LinkUpWidgetState_t *)context;
  state->child_type_key = get_next_key2(state->child_type_key);
  m_set_state(state->context);
}

static void build_children(LinkUpWidgetState_t *const state) {
  uint32_t key =
      (uint32_t)(state->refresh_key) << 16 | (uint16_t)(state->child_type_key);
  switch (state->child_type_key) {
  case ChildType_ControlInfo: {
    state->animated_transition_widget_data = (AnimatedTransitionWidgetData_t){
        .div = state->div,
        .key = key,
        .children =
            {
                {
                    .class = &ControlInfoWidgetClass,
                    .data = state,
                },
                NullWidget,
                NullWidget,
                NullWidget,
            },
    };
    break;
  }
  case ChildType_GForce: {
    state->animated_transition_widget_data = (AnimatedTransitionWidgetData_t){
        .div = state->div,
        .key = key,
        .children =
            {
                {
                    .class = &GForceWidgetClass,
                    .data = state,
                },
                NullWidget,
                NullWidget,
                NullWidget,
            },
    };
    break;
  }
  case ChildType_Tachometer:
  default: {
    state->animated_transition_widget_data = (AnimatedTransitionWidgetData_t){
        .div = state->div,
        .key = key,
        .children =
            {
                {
                    .class = &TachometerLedControllerWidgetClass,
                    .data = state,
                },
                {
                    .class = &TachometerWidgetClass,
                    .data = state,
                },
                NullWidget,
                NullWidget,
            },
    };
    break;
  }
  }
}

static void *init_state(mContext_t *context) {
  assert(state.div == NULL);
  assert(state.context == NULL);
  state.is_dirty = false;

  m_get_widget_data_cast(main_widget_state, context, MainWidgetState_t);

  const int32_t width = lv_obj_get_width(main_widget_state->screen);
  const int32_t height = lv_obj_get_height(main_widget_state->screen);

  lv_obj_t *const div = lv_obj_create(main_widget_state->screen);
  lv_obj_set_size(div, width, height);
  lv_obj_set_flex_flow(div, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(div, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scrollable(div, false);
  lv_obj_set_style_pad_all(div, 0, 0);
  lv_obj_set_pos(div, 0, main_widget_state->is_link_state_up ? 0 : -height);

  lv_anim_t div_anim;
  lv_anim_init(&div_anim);
  lv_anim_set_var(&div_anim, div); // bind anim, delete anim in [dispose]
  lv_anim_set_values(&div_anim, -height, 0);
  lv_anim_set_custom_exec_cb(&div_anim, div_anim_callback);
  lv_anim_set_path_cb(&div_anim, lv_anim_path_ease_in_out);
  lv_anim_set_duration(&div_anim, 100);
  lv_anim_timeline_add(main_widget_state->anim_timeline, 200, &div_anim);

  state.k1_listener = (EventTask_t){
      .callback = &on_k1,
      .context = &state,
  };
  add_task(&k1_listeners, &state.k1_listener);

  state.k2_listener = (EventTask_t){
      .callback = &on_k2,
      .context = &state,
  };
  add_task(&k2_listeners, &state.k2_listener);

  state.recv_task = (DataRecvTask_t){
      .callback = &data_recv_callback,
      .context = &state,
  };
  init(&state.data_recv_listeners);

  state.circular_buffer = &circular_buffer;
  state.data_analysis = &data_analysis;
  state.context = context;
  state.div = div;
  clear_data(&state);

  state.refresh_key = 0;
  state.child_type_key = ChildType_Tachometer;
  build_children(&state);

  return &state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  m_get_widget_data_cast(data, context, MainWidgetState_t);
  m_get_state_cast(state, context, LinkUpWidgetState_t);

  children[0] = (mWidget_t){
      .class = data->is_link_state_up ? &UdpServerWidgetClass : NULL,
      .data = &state->recv_task,
  };

  build_children(state);
  children[1] = (mWidget_t){
      .class = &AnimatedTransitionWidgetClass,
      .data = &state->animated_transition_widget_data,
  };

  return;
}

static void dispose(mContext_t *context) {
  m_get_state_cast(state, context, LinkUpWidgetState_t);
  remove_task(&k2_listeners, &state->k2_listener);
  remove_task(&k1_listeners, &state->k1_listener);
  lv_anim_del(state->div, NULL);
  lv_obj_del(state->div);
  cleanup(&state->data_recv_listeners);
  state->div = NULL;
  state->context = NULL;
  state->circular_buffer = NULL;
  state->is_dirty = false;
  return;
};

const mWidgetClass_t LinkUpWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};

DataPacket_t *circular_buffer_push(CircularBuffer_t *buffer) {
#define ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))
  if (buffer->length >= ARRAY_LEN(buffer->data_packets)) {
    buffer->length = ARRAY_LEN(buffer->data_packets);
    uint16_t ret_index = buffer->start;
    buffer->start++;
    if (buffer->start == ARRAY_LEN(buffer->data_packets)) {
      buffer->start = 0;
    }
    return &buffer->data_packets[ret_index];
  } else {
    uint16_t ret_index =
        (buffer->start + buffer->length) % ARRAY_LEN(buffer->data_packets);
    buffer->length++;
    return &buffer->data_packets[ret_index];
  }
}
const DataPacket_t *circular_buffer_get(const CircularBuffer_t *buffer,
                                        size_t index) {
  uint16_t ret_index =
      (buffer->start + index) % ARRAY_LEN(buffer->data_packets);
  return &buffer->data_packets[ret_index];
}

const DataPacket_t *circular_buffer_get_last(const CircularBuffer_t *buffer) {
  if (buffer->length == 0) {
    return NULL;
  }
  return circular_buffer_get(buffer, buffer->length - 1);
}

void circular_buffer_clear(CircularBuffer_t *buffer) {
  buffer->start = 0;
  buffer->length = 0;
}

void data_analysis_clear(DataAnalysis_t *data_analysis) {
  memset(data_analysis, 0, sizeof(*data_analysis));
}
