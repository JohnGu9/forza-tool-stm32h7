#ifndef MF_H
#define MF_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MF_MAX_CHILDREN
#define MF_MAX_CHILDREN 4
#endif

typedef struct _mWidgetClass mWidgetClass_t;
typedef struct _mWidget mWidget_t;
typedef struct _mContext mContext_t;

struct _mWidget {
  const mWidgetClass_t *class; // can not be null
  void *data;
};

struct _mWidgetClass {
  void *(*init_state)(mContext_t *context);
  void (*build)(mContext_t *context, mWidget_t children[MF_MAX_CHILDREN]);
  void (*dispose)(mContext_t *context);
};

struct _mContext {
  // internal use
  // user should not access members directly
  mContext_t *(*new)();
  void (*delete)(mContext_t *);
  void (*on_error)(const char *reason);
  const mWidget_t *widget;
  void *state;
  const mContext_t *parent;
  size_t index;
  mWidget_t children[MF_MAX_CHILDREN];
  mContext_t *children_context[MF_MAX_CHILDREN];
  int status;
};

extern const mWidget_t NullWidget;

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

void *
m_get_state_from_inherited_widget_class(const mContext_t *context,
                                        const mWidgetClass_t *widget_class);

#ifndef m_get_state_cast
#define m_get_state_cast(state, context, T)                                    \
  T *const state = (T *)m_get_state((context))
#endif

#ifndef m_get_widget_data_cast
#define m_get_widget_data_cast(data, context, T)                               \
  const T *const data = (const T *)m_get_widget_data((context))
#endif

#ifndef m_get_state_from_inherited_widget_class_cast
#define m_get_state_from_inherited_widget_class_cast(inherited_widget_state,   \
                                                     context, widget_class, T) \
  const T *const inherited_widget_state =                                      \
      (const T *)m_get_state_from_inherited_widget_class((context),            \
                                                         (widget_class))
#endif

#ifdef __cplusplus
}
#endif

#endif
