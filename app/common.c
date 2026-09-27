#include "common.h"

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
