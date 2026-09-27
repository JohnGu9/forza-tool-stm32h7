#include "mf.h"

#include <malloc.h>
#include <stdio.h>
#include <stdint.h>

typedef struct _MyWidgetChildState {
  int id;
} MyWidgetChildState;

static void *my_widget_child_init_state(mContext_t *context) {
  MyWidgetChildState *state =
      (MyWidgetChildState *)malloc(sizeof(MyWidgetChildState));
  const void *widget_data = m_get_widget_data(context);
  printf("my_widget_child_init_state: %p : %p\n", widget_data, state);
  return state;
}

static void my_widget_child_build(mContext_t *context,
                                  mWidget_t children[MF_MAX_CHILDREN]) {
  const void *widget_data = m_get_widget_data(context);
  printf("my_widget_child_build: %p\n", widget_data);
}

static void my_widget_child_dispose(mContext_t *context) {
  const void *widget_data = m_get_widget_data(context);
  m_get_state_cast(state, context, MyWidgetChildState);
  printf("my_widget_child_dispose: %p : %p\n", widget_data, state);
  free(state);
}

static const mWidgetClass_t MyWidgetClass1 = {
    .init_state = &my_widget_child_init_state,
    .build = &my_widget_child_build,
    .dispose = &my_widget_child_dispose,
};

typedef struct _myWidgetState_t {
  void *unused;
} MyWidgetState;

static void *my_widget_init_state(mContext_t *context) {
  MyWidgetState *state = (MyWidgetState *)malloc(sizeof(MyWidgetState));
  printf("my_widget_init_state\n");
  return state;
}

static int switch_key = 0;

static void my_widget_build(mContext_t *context,
                            mWidget_t children[MF_MAX_CHILDREN]) {
  printf("my_widget_build\n");
  m_get_state_cast(state, context, MyWidgetState);

  children[0] = (mWidget_t) {
    .class = &MyWidgetClass1,
    .data = (void *)0,
  };
  children[1] = (mWidget_t) {
    .class = switch_key ? &MyWidgetClass1 : NULL,
    .data = (void *)1,
  };
}

static void my_widget_dispose(mContext_t *context) {
  printf("my_widget_dispose\n");
  m_get_state_cast(state, context, MyWidgetState);
  free(state);
}

static const mWidgetClass_t MyWidgetClass = {
    .init_state = &my_widget_init_state,
    .build = &my_widget_build,
    .dispose = &my_widget_dispose,
};

static mContext_t *new_context() {
  mContext_t *context = (mContext_t *)malloc(sizeof(mContext_t));
  printf("new_context: %p\n", context);
  return context;
}

static void delete_context(mContext_t *context) {
  printf("delete_context: %p\n", context);
  free(context);
}

static void on_error(const char *reason) { printf("%s\n", reason); }

struct A {
  int a;
};

typedef mContext_t * mContextPtr_t;

int main() {
  mContext_t context = {0}; // ensure root context memory is clear before attach

  const mWidget_t my_widget = {
      .class = &MyWidgetClass,
      .data = NULL,
  };

  printf("m_attach\n\n");
  m_attach(&context, &my_widget, new_context, delete_context, on_error);

  printf("\nm_set_state switch_key = 1\n\n");
  switch_key = 1;
  m_set_state(&context);

  printf("\nm_set_state switch_key = 0\n\n");
  switch_key = 0;
  m_set_state(&context);

  printf("\nm_detach\n\n");
  m_detach(&context);

  return 0;
}
