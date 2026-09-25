#include "mf.h"
#include <stdlib.h>

typedef struct {
} MyWidgetState_t;

static void *init_state(mContext_t *context) {
  MyWidgetState_t *state = malloc(sizeof(MyWidgetState_t));
  return state;
}

static void build(mContext_t *context, void *state,
                  const mWidget_t *children[MF_MAX_CHILDREN],
                  const void *children_widget_data[MF_MAX_CHILDREN]) {
  return;
}

static void dispose(mContext_t *context, void *state) {
  free(state);
  return;
};

const mWidget_t MyWidget = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
