#include "widgets.h"

#include "io.h"

#include <lvgl.h>
#include <stdlib.h>

static MainWidgetState_t state = {0};

static void link_state_change_task_callback(void *context) {
  MainWidgetState_t *state = (MainWidgetState_t *)context;
  m_set_state(state->context);
}

static void *init_state(mContext_t *context) {
  assert(state.screen == NULL);
  assert(state.context == NULL);
  assert(state.anim_timeline == NULL);

  const int32_t width = lv_display_get_original_horizontal_resolution(display);
  const int32_t height = lv_display_get_original_vertical_resolution(display);

  lv_obj_t *screen = lv_display_get_screen_active(display);
  lv_obj_set_pos(screen, 0, 0);
  lv_obj_set_size(screen, width, height);
  lv_obj_set_scrollable(screen, false);
  lv_obj_update_layout(screen);

  state.screen = screen;
  state.context = context;
  state.is_link_state_up = is_link_state_up();
  state.anim_timeline = lv_anim_timeline_create();

  lv_anim_timeline_set_progress(
      state.anim_timeline,
      state.is_link_state_up ? 0 : LV_ANIM_TIMELINE_PROGRESS_MAX);

  state.link_state_change_task.callback = &link_state_change_task_callback;
  state.link_state_change_task.context = &state;
  add_link_state_listener(&state.link_state_change_task);

  return &state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  m_get_state_cast(state, context, MainWidgetState_t);
  bool is_link_up = is_link_state_up();
  if (is_link_up != state->is_link_state_up) {
    lv_anim_timeline_set_reverse(state->anim_timeline, !is_link_up);
    lv_anim_timeline_start(state->anim_timeline);
    state->is_link_state_up = is_link_up;
  }

  children[0] = (mWidget_t){
      .class = &LinkUpWidgetClass,
      .data = state,
  };
  children[1] = (mWidget_t){
      .class = &LinkDownWidgetClass,
      .data = state,
  };

  return;
}

static void dispose(mContext_t *context) {
  m_get_state_cast(state, context, MainWidgetState_t);

  remove_link_state_listener(&state->link_state_change_task);
  lv_anim_timeline_delete(state->anim_timeline);

  state->anim_timeline = NULL;
  state->screen = NULL;
  state->context = NULL;
  return;
};

const mWidgetClass_t MainWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
