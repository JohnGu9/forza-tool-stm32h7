#include "mf.h"

#include <string.h>

typedef enum _mContextStatus {
  NONE,
  CHANGING,
} mContextStatus_t;

#define MemoryAllocFailedReason "Memory alloc failed. "
#define FunctionCallRecursivelyReason                                          \
  "Function [m_attach|m_detach|init_state|m_set_state|dispose] can not "       \
  "be called recursively. "

static void mask_context_is_changing(mContext_t *context) {
  if (context->status == CHANGING) {
    context->on_error(FunctionCallRecursivelyReason);
  }

  context->status = CHANGING;
}

void m_attach(mContext_t *context, const mWidget_t *widget,
              mContext_t *(*new)(), void (*delete)(mContext_t *),
              void (*on_error)(const char *reason)) {
  memset(context->children, 0, sizeof(context->children));
  memset(context->children_context, 0, sizeof(context->children_context));

  context->new = new;
  context->delete = delete;
  context->on_error = on_error;
  context->widget = widget;

  context->status = CHANGING;
  context->state = context->widget->init_state(context);
  context->widget->build(context, context->state,
                         (const mWidget_t **)context->children,
                         context->children_widget_data);
  for (size_t i = 0; i < MF_MAX_CHILDREN; i++) {
    mWidget_t *const child = context->children[i];
    if (child != NULL) {
      mContext_t *child_context = new();
      if (child_context != NULL) {
        context->children_context[i] = child_context;
        child_context->index = i;
        child_context->parent = context;
        m_attach(child_context, child, new, delete, on_error);
      } else {
        on_error(MemoryAllocFailedReason);
      }
    }
  }
  context->status = NONE;
}

static void m_detach_internal(mContext_t *parent_context, size_t childIndex) {
  mContext_t *child_context = parent_context->children_context[childIndex];
  if (child_context != NULL) {
    mask_context_is_changing(child_context);
    for (size_t i = MF_MAX_CHILDREN; i != 0; i--) {
      m_detach_internal(child_context, i - 1);
    }
    child_context->widget->dispose(child_context, child_context->state);
    parent_context->delete(child_context);
  }
  parent_context->children[childIndex] = NULL;
  parent_context->children_context[childIndex] = NULL;
}

void m_set_state(mContext_t *context) {
  mWidget_t *new_children[MF_MAX_CHILDREN];
  memcpy(new_children, context->children, sizeof(new_children));

  mask_context_is_changing(context);
  context->widget->build(context, context->state,
                         (const mWidget_t **)new_children,
                         context->children_widget_data);
  for (size_t i = 0; i < MF_MAX_CHILDREN; i++) {
    mWidget_t *const child = context->children[i];
    mWidget_t *const new_child = new_children[i];
    if (new_child != child) {
      if (child != NULL) {
        m_detach_internal(context, i);
      }
      if (new_child != NULL) {
        mContext_t *child_context = context->new();
        if (child_context != NULL) {
          context->children[i] = new_child;
          context->children_context[i] = child_context;
          child_context->index = i;
          child_context->parent = context;
          m_attach(child_context, new_child, context->new, context->delete,
                   context->on_error);
        } else {
          context->on_error(MemoryAllocFailedReason);
        }
      }
    } else {
      if (context->children_context[i] != NULL) {
        m_set_state(context->children_context[i]);
      }
    }
  }
  context->status = NONE;
}

void m_detach(mContext_t *context) {
  mask_context_is_changing(context);
  for (size_t i = MF_MAX_CHILDREN; i != 0; i--) {
    m_detach_internal(context, i - 1);
  }
  context->widget->dispose(context, context->state);
}

void *m_get_state(const mContext_t *context) { return context->state; }

const mWidget_t *m_get_widget(const mContext_t *context) {
  return context->widget;
}

const void *m_get_widget_data(const mContext_t *context) {
  const mContext_t *parent_context = context->parent;
  if (parent_context != NULL) {
    return parent_context->children_widget_data[context->index];
  }
  return NULL;
}
