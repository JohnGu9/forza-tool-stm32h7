#include "mf.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

enum {
  NONE,
  CHANGING,
};

#define MemoryAllocFailedReason "Memory alloc failed. "
#define FunctionCallRecursivelyReason                                          \
  "Function [m_attach|m_detach|init_state|m_set_state|dispose] can not "       \
  "be called recursively. "

const mWidget_t NullWidget = {
    .class = NULL,
    .data = NULL,
};

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
  context->state = context->widget->class->init_state(context);
  context->widget->class->build(context, context->children);
  for (size_t i = 0; i < MF_MAX_CHILDREN; i++) {
    mWidget_t *const child = &context->children[i];
    if (child->class != NULL) {
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
    child_context->widget->class->dispose(child_context);
    parent_context->delete(child_context);
  }
  parent_context->children[childIndex].class = NULL;
  parent_context->children[childIndex].data = NULL;
  parent_context->children_context[childIndex] = NULL;
}

void m_set_state(mContext_t *context) {
  mWidget_t new_children[MF_MAX_CHILDREN] = {0};

  mask_context_is_changing(context);
  context->widget->class->build(context, new_children);
  for (size_t i = 0; i < MF_MAX_CHILDREN; i++) {
    mWidget_t *const child = &context->children[i];
    mWidget_t *const new_child = &new_children[i];
    if (child->class != new_child->class) {
      if (child->class != NULL) {
        m_detach_internal(context, i);
      }
      if (new_child->class != NULL) {
        mContext_t *child_context = context->new();
        if (child_context != NULL) {
          *child = *new_child;
          context->children_context[i] = child_context;
          child_context->index = i;
          child_context->parent = context;
          m_attach(child_context, child, context->new, context->delete,
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
  context->widget->class->dispose(context);
}

void *m_get_state(const mContext_t *context) { return context->state; }

const mWidget_t *m_get_widget(const mContext_t *context) {
  return context->widget;
}

const void *m_get_widget_data(const mContext_t *context) {
  return context->widget->data;
}

void *
m_get_state_from_inherited_widget_class(const mContext_t *context,
                                        const mWidgetClass_t *widget_class) {
  for (const mContext_t *parent = context->parent; parent != NULL;
       parent = parent->parent) {
    if (m_get_widget(parent)->class == widget_class) {
      return m_get_state(parent);
    }
  }
  return NULL;
}
