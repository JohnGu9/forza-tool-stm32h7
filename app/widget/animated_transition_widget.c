#include "animated_transition_widget.h"

#include "common.h"

#include <assert.h>
#include <lvgl/core/lv_obj_pos.h>

// @TODO: more style

typedef enum {
  Enterred,
  Exiting,
  Exited,
  Entering,
} AnimatedTransitionState;

typedef struct {
  mContext_t *context;
  AnimatedTransitionState state;
  uintptr_t key;
  mWidget_t children[MF_MAX_CHILDREN];
} AnimatedTransitionWidgetState_t;

static void exit_anim_callback(void *var, int32_t v) { // v = 0 -> 1000
  lv_obj_t *const div = (lv_obj_t *)var;
  float progress = (float)v / 1000.0;
  lv_obj_set_style_translate_x(div, progress * lv_obj_get_width(div),
                               LV_PART_MAIN);
}

static void on_exited(lv_anim_t *var) {
  lv_obj_t *const div = (lv_obj_t *)var->var;
  AnimatedTransitionWidgetState_t *const state =
      (AnimatedTransitionWidgetState_t *)lv_obj_get_user_data(div);
  state->state = Exited;
  m_set_state(state->context);
}

static void enter_anim_callback(void *var, int32_t v) { // v = 0 -> 1000
  lv_obj_t *const div = (lv_obj_t *)var;
  float progress = (float)v / 1000.0;
  lv_obj_set_style_translate_x(div, -(1 - progress) * lv_obj_get_width(div),
                               LV_PART_MAIN);
}

static void on_entered(lv_anim_t *var) {
  lv_obj_t *const div = (lv_obj_t *)var->var;
  AnimatedTransitionWidgetState_t *const state =
      (AnimatedTransitionWidgetState_t *)lv_obj_get_user_data(div);
  state->state = Enterred;
  m_set_state(state->context);
}

static void *init_state(mContext_t *context) {
  m_get_widget_data_cast(data, context, AnimatedTransitionWidgetData_t);
  AnimatedTransitionWidgetState_t *const state =
      new_object(AnimatedTransitionWidgetState_t);
  state->state = Enterred;
  state->context = context;
  state->key = data->key;
  memcpy(state->children, data->children, sizeof(state->children));

  lv_obj_t *const div = data->div;
  assert(lv_obj_get_user_data(div) == NULL);
  lv_obj_set_user_data(div, state);

  return state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  m_get_widget_data_cast(data, context, AnimatedTransitionWidgetData_t);
  m_get_state_cast(state, context, AnimatedTransitionWidgetState_t);
  switch (state->state) {
  case Exiting: {
    break;
  }
  case Exited: {
    state->key = data->key;
    lv_anim_t enter_anim;
    lv_anim_init(&enter_anim);
    lv_anim_set_var(&enter_anim, data->div);
    lv_anim_set_values(&enter_anim, 0, 1000);
    lv_anim_set_exec_cb(&enter_anim, enter_anim_callback);
    lv_anim_set_path_cb(&enter_anim, lv_anim_path_ease_in);
    lv_anim_set_duration(&enter_anim, 200);
    lv_anim_set_completed_cb(&enter_anim, on_entered);
    lv_anim_start(&enter_anim);
    state->state = Entering;
    break;
  }
  case Entering: {
    break;
  }
  case Enterred:
  default: {
    if (unlikely(state->key != data->key)) {
      state->state = Exiting;
      lv_anim_t exit_anim;
      lv_anim_init(&exit_anim);
      lv_anim_set_var(&exit_anim, data->div);
      lv_anim_set_values(&exit_anim, 0, 1000);
      lv_anim_set_exec_cb(&exit_anim, exit_anim_callback);
      lv_anim_set_path_cb(&exit_anim, lv_anim_path_ease_out);
      lv_anim_set_duration(&exit_anim, 100);
      lv_anim_set_completed_cb(&exit_anim, on_exited);
      lv_anim_start(&exit_anim);
      state->state = Exiting;
    }
    break;
  }
  }
  if (state->key == data->key) {
    memcpy(state->children, data->children, sizeof(state->children));
  }
  memcpy(children, state->children, sizeof(state->children));
  return;
}

static void dispose(mContext_t *context) {
  m_get_widget_data_cast(data, context, AnimatedTransitionWidgetData_t);
  m_get_state_cast(state, context, AnimatedTransitionWidgetState_t);
  lv_anim_del(data->div, NULL);
  delete_object(state);
  return;
};

const mWidgetClass_t AnimatedTransitionWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
