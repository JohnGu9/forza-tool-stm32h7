#ifndef ANIMATED_TRANSITION_WIDGET_H
#define ANIMATED_TRANSITION_WIDGET_H

#include "mf.h"

#include <lvgl.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uintptr_t key;
  lv_obj_t *div;
  mWidget_t children[MF_MAX_CHILDREN];
} AnimatedTransitionWidgetData_t;

extern const mWidgetClass_t AnimatedTransitionWidgetClass;

#ifdef __cplusplus
}
#endif

#endif
