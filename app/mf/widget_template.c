#include "mf.h"
#include <stdlib.h>

typedef struct {
} MyWidgetState_t;

static void *init_state(mContext_t *context) {
  MyWidgetState_t *state = (MyWidgetState_t *)malloc(sizeof(MyWidgetState_t));
  return state;
}

static void build(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]) {
  return;
}

static void dispose(mContext_t *context) {
  m_get_state_cast(state, context, MyWidgetState_t);
  free(state);
  return;
};

const mWidgetClass_t MyWidgetClass = {
    .init_state = &init_state,
    .build = &build,
    .dispose = &dispose,
};
