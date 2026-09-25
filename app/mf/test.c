#include "mf.h"

#include <malloc.h>
#include <stdio.h>

struct MyWidgetState {
  int id;
};

static void *my_widget1_init_state(mContext_t *context) {
  struct MyWidgetState *state =
      (struct MyWidgetState *)malloc(sizeof(struct MyWidgetState));
  const void *widget_data = m_get_widget_data(context);
  printf("my_widget1_init_state: %p : %p\n", widget_data, state);
  return state;
}

static void
my_widget1_build(mContext_t *context, void *state,
                 const mWidget_t *children[MF_MAX_CHILDREN],
                 const void *children_widget_data[MF_MAX_CHILDREN]) {
  printf("my_widget1_build\n");
}

static void my_widget1_dispose(mContext_t *context, void *state) {
  const void *widget_data = m_get_widget_data(context);

  printf("my_widget1_dispose: %p : %p\n", widget_data, state);
  free(state);
}

static const mWidget_t my_widget1 = {
    .init_state = &my_widget1_init_state,
    .build = &my_widget1_build,
    .dispose = &my_widget1_dispose,
};

static void *my_widget_init_state(mContext_t *context) {
  printf("my_widget_init_state\n");
  return NULL;
}

static int switch_key = 0;

static void my_widget_build(mContext_t *context, void *state,
                            const mWidget_t *children[MF_MAX_CHILDREN],
                            const void *children_widget_data[MF_MAX_CHILDREN]) {
  printf("my_widget_build\n");
  children[0] = &my_widget1;
  children_widget_data[0] = (void *)0;

  if (switch_key) {
    children[1] = &my_widget1;
  } else {
    children[1] = NULL;
  }
  children_widget_data[1] = (void *)1;
}

static void my_widget_dispose(mContext_t *context, void *state) {
  printf("my_widget_dispose\n");
}

static const mWidget_t my_widget = {
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

int main() {
  printf("Hello\n");

  mContext_t context = {0}; // ensure root context memory is clear before attach

  m_attach(&context, &my_widget, new_context, delete_context, on_error);

  printf("m_attach\n\n");

  switch_key = 1;
  m_set_state(&context);

  printf("m_set_state\n\n");

  switch_key = 0;
  m_set_state(&context);

  printf("m_set_state\n\n");

  m_detach(&context);

  return 0;
}
