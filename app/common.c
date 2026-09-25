#include "common.h"

static_assert(sizeof(Fh4Data_t) == 323);
static_assert(sizeof(Fm7Data_t) == 311);
static_assert(sizeof(Fm8Data_t) == 331);

// @TODO: use set container

void invoke_tasks(EventTask_t *list[], size_t length) {
  for (size_t i = 0; i < length; i++) {
    const EventTask_t *task = list[i];
    if (task != NULL) {
      task->callback(task->context);
    }
  }
}

int32_t add_task(EventTask_t *task, EventTask_t *list[], size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (list[i] == task) {
      return 1;
    }
  }
  for (size_t i = 0; i < length; i++) {
    if (list[i] == NULL) {
      list[i] = task;
      return 0;
    }
  }
  return -1;
}

int32_t remove_task(EventTask_t *task, EventTask_t *list[], size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (list[i] == task) {
      list[i] = NULL;
      return 0;
    }
  }
  return -1;
}

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
