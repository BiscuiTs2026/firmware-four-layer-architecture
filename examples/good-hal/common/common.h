#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/* 统一错误码：任何层的公共 API 只返回这组值 */
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
