#ifndef PRACTICE_TYPES_H
#define PRACTICE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  ERR_OK = 0,
  ERR_PARAM,
  ERR_NULL,
  ERR_RANGE,
  ERR_TIMEOUT,
  ERR_BUSY,
  ERR_IO,
  ERR_STATE,
  ERR_NOMEM
} err_t;

#endif
