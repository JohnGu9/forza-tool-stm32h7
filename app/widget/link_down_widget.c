#include "widgets.h"

#include "mf.h"

#include <lvgl.h>
#include <stdlib.h>

static void set_div_pos_y(lv_anim_t *var, int32_t v) {
  lv_obj_t *const div = (lv_obj_t *)var->var;
  lv_obj_set_y(div, v);
}

typedef struct {
  lv_obj_t *div;
} LinkDownWidgetState_t;

static LinkDownWidgetState_t state = {0};

static void *init_state(mContext_t *context) {
  assert(state.div == NULL);

  const MainWidgetState_t *const main_widget_state =
      (const MainWidgetState_t *)m_get_widget_data(context);

  const int32_t width = lv_obj_get_width(main_widget_state->screen);
  const int32_t height = lv_obj_get_height(main_widget_state->screen);

  lv_obj_t *const div = lv_obj_create(main_widget_state->screen);
  lv_obj_set_size(div, width, height);
  lv_obj_set_flex_flow(div, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(div, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_pos(div, 0, main_widget_state->is_link_state_up ? height : 0);
  lv_obj_set_scrollable(div, false);

  lv_anim_t div_anim;
  lv_anim_init(&div_anim);
  lv_anim_set_var(&div_anim, div); // bind anim, delete anim in [dispose]
  lv_anim_set_values(&div_anim, 0, height);
  lv_anim_set_custom_exec_cb(&div_anim, set_div_pos_y);
  lv_anim_set_path_cb(&div_anim, lv_anim_path_ease_in_out);
  lv_anim_set_duration(&div_anim, 300);
  lv_anim_timeline_add(main_widget_state->anim_timeline, 0, &div_anim);

  lv_obj_t *const spinner = lv_spinner_create(div);
  const int32_t spinner_size = (MIN(width, height)) / 2;
  lv_obj_set_size(spinner, spinner_size, spinner_size);
  lv_spinner_set_anim_duration(spinner, 1500);
  lv_spinner_set_arc_sweep(spinner, 270);

  lv_obj_t *label0 = lv_label_create(div);
  lv_label_set_text(label0, "USB\nCONNECTING");

  state.div = div;
  return &state;
}

static void build(mContext_t *context, void *state,
                  const mWidget_t *children[MF_MAX_CHILDREN],
                  const void *children_widget_data[MF_MAX_CHILDREN]) {
  return;
}

static void dispose(mContext_t *context, void *_state) {
  LinkDownWidgetState_t *state = (LinkDownWidgetState_t *)_state;
  lv_anim_del(state->div, NULL);
  lv_obj_del(state->div);
  state->div = NULL;
  return;
}

WIDGET_MEMORY_LOCATION const mWidget_t LinkDownWidget = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
