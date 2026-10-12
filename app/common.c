#include "common.h"
#include <string.h>

int32_t add_task(EventTaskSet_t *set, EventTask_t *task) {
  return !insert(set, (uintptr_t)task);
}

int32_t remove_task(EventTaskSet_t *set, EventTask_t *task) {
  erase(set, (uintptr_t)task);
  return 0;
}

void invoke_tasks(EventTaskSet_t *set) {
  for_each(set, el) {
    EventTask_t *task = (EventTask_t *)(*el);
    task->callback(task->context);
  }
}

static_assert(sizeof(Fh4Data_t) == 323);
static_assert(sizeof(Fm7Data_t) == 311);
static_assert(sizeof(Fm8Data_t) == 331);

const SledData_t *get_sled_data(const uint8_t *buffer, const size_t length) {
  switch (length) {
  case 323:
  case 324: {
    const Fh4Data_t *data = (const Fh4Data_t *)buffer;
    return &data->sled;
  }

  case 311:
  case 312: {
    const Fm7Data_t *data = (const Fm7Data_t *)buffer;
    return &data->sled;
  }

  case 331:
  case 332: {
    const Fm8Data_t *data = (const Fm8Data_t *)buffer;
    return &data->sled;
  }
  }
  return NULL;
}

const DashData_t *get_dash_data(const uint8_t *buffer, const size_t length) {
  switch (length) {
  case 323:
  case 324: {
    const Fh4Data_t *data = (const Fh4Data_t *)buffer;
    return &data->dash;
  }

  case 311:
  case 312: {
    const Fm7Data_t *data = (const Fm7Data_t *)buffer;
    return &data->dash;
  }

  case 331:
  case 332: {
    const Fm8Data_t *data = (const Fm8Data_t *)buffer;
    return &data->dash;
  }
  }
  return NULL;
}

typedef map(uintptr_t, size_t) cc_mem_table_t;

static cc_mem_table_t *cc_mem_table = NULL;
static size_t cc_mem_table_size = 0;

static void *cc_mem_table_realloc(void *ptr, size_t size) {
  void *new_ptr = pvPortMalloc(size);
  if (unlikely(new_ptr == NULL)) {
    return NULL;
  }
  if (ptr != NULL) {
    size_t copy_size = MIN(cc_mem_table_size, size);
    memcpy(new_ptr, ptr, copy_size);
    vPortFree(ptr);
  }
  cc_mem_table_size = size;
  return new_ptr;
}

static void cc_mem_table_free(void *ptr) {
  cc_mem_table_size = 0;
  vPortFree(ptr);
}

#undef CC_REALLOC
#define CC_REALLOC cc_mem_table_realloc
#undef CC_FREE
#define CC_FREE cc_mem_table_free

void *our_realloc(void *ptr, size_t size) {
  if (unlikely(cc_mem_table == NULL)) {
    static cc_mem_table_t table;
    cc_mem_table = &table;
    init(cc_mem_table);
  }
  void *new_ptr = pvPortMalloc(size);
  if (unlikely(new_ptr == NULL)) {
    return NULL;
  }
  if (ptr != NULL) {
    size_t *old_size = get(cc_mem_table, (uintptr_t)ptr);
    if (likely(old_size != NULL)) {
      size = MIN(*old_size, size);
      erase(cc_mem_table, (uintptr_t)ptr);
    } else {
      // @TODO: error handling
      // missing size info
      // insert failed before
    }
    memcpy(new_ptr, ptr, size);
    vPortFree(ptr);
  }
  insert(cc_mem_table, (uintptr_t)new_ptr, size);
  return new_ptr;
}

void our_free(void *ptr) {
  if (likely(ptr != NULL && cc_mem_table != NULL)) {
    erase(cc_mem_table, (uintptr_t)ptr);
  }
  vPortFree(ptr);
}
