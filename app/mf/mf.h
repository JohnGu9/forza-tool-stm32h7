#ifndef MF_H
#define MF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MF_MAX_CHILDREN
#define MF_MAX_CHILDREN 8
#endif

typedef struct _mWidget mWidget_t;
typedef struct _mContext mContext_t;

struct _mContext {
  // internal use
  // user should not access members manually
  mContext_t *(*new)();
  void (*delete)(mContext_t *);
  void (*on_error)(const char *reason);
  const mWidget_t *widget;
  void *state;
  mWidget_t *children[MF_MAX_CHILDREN];
  const void *children_widget_data[MF_MAX_CHILDREN];
  const mContext_t *parent;
  size_t index;
  mContext_t *children_context[MF_MAX_CHILDREN];
  int status;
};

struct _mWidget {
  void *(*init_state)(mContext_t *context);
  void (*build)(mContext_t *context, void *state,
                const mWidget_t *children[MF_MAX_CHILDREN],
                const void *children_widget_data[MF_MAX_CHILDREN]);
  void (*dispose)(mContext_t *context, void *state);
};

// top level function

void m_attach(mContext_t *context, const mWidget_t *widget,
              mContext_t *(*new)(), void (*delete)(mContext_t *),
              void (*on_error)(const char *reason));

void m_detach(mContext_t *context);

// helper function

void m_set_state(mContext_t *context);

void *m_get_state(const mContext_t *context);

const mWidget_t *m_get_widget(const mContext_t *context);

const void *m_get_widget_data(const mContext_t *context);

#ifdef __cplusplus
}
#endif

#endif
