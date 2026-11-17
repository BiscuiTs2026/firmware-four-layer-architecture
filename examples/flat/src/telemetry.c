#include "telemetry.h"

#include <string.h>

#include "board_config.h"

#define PID_SCALE      1000
#define PID_OUT_LIMIT  1000000
#define PID_INT_LIMIT  1000000

typedef struct
{
  int32_t kp;
  int32_t ki;
  int32_t kd;
} gain_t;

/* 全部为 static，头文件不暴露任何内部状态 */
static gain_t s_gain = { 2000, 0, 0 };
static int32_t s_target;
static int32_t s_actual;
static int32_t s_output;
static int32_t s_integ;
static int32_t s_prev_err;
static uint32_t s_phase;

static int32_t clamp(int32_t v, int32_t lo, int32_t hi)
{
  if (v > hi)
  {
    return hi;
  }
  if (v < lo)
  {
    return lo;
  }
  return v;
}

/* 毫单位定点数转文本，例如 -1234 -> "-1.234"；返回写入后的位置 */
static uint16_t append_milli(char *out, uint16_t pos, uint16_t cap, int32_t value)
{
  char tmp[12];
  uint8_t n = 0U;
  uint32_t mag;
  uint32_t frac;
  uint32_t whole;

  if ((uint32_t)pos + 10U >= cap)
  {
    return pos;
  }

  if (value < 0)
  {
    out[pos++] = '-';
    mag = (uint32_t)(-(value + 1)) + 1U;
  }
  else
  {
    mag = (uint32_t)value;
  }

  frac = mag % (uint32_t)PID_SCALE;
  whole = mag / (uint32_t)PID_SCALE;

  do
  {
    tmp[n++] = (char)('0' + (whole % 10U));
    whole /= 10U;
  } while ((whole != 0U) && (n < (uint8_t)sizeof(tmp)));

  while (n > 0U)
  {
    out[pos++] = tmp[--n];
  }

  out[pos++] = '.';
  out[pos++] = (char)('0' + ((frac / 100U) % 10U));
  out[pos++] = (char)('0' + ((frac / 10U) % 10U));
  out[pos++] = (char)('0' + (frac % 10U));
  out[pos] = '\0';
  return pos;
}

void telemetry_init(void)
{
  s_target = 5000;
  s_actual = 0;
  s_output = 0;
  s_integ = 0;
  s_prev_err = 0;
  s_phase = 0U;
}

void telemetry_sample(void)
{
  int32_t err;

  /* 目标值：0.5 s 上升 / 0.5 s 下降的三角波，单位毫 */
  s_phase = (s_phase + 1U) % 2000U;
  if (s_phase < 1000U)
  {
    s_target = (int32_t)(5000U + s_phase * 10U);
  }
  else
  {
    s_target = (int32_t)(15000U - (s_phase - 1000U) * 10U);
  }

  /* 一阶惯性模拟被控对象，替代真实传感器 */
  s_actual += (s_target - s_actual) / 32;
  err = s_target - s_actual;

  s_output = (s_gain.kp * err) / PID_SCALE;
  s_integ = clamp(s_integ + (s_gain.ki * err) / 100000, -PID_INT_LIMIT, PID_INT_LIMIT);
  s_output = s_output + s_integ + (s_gain.kd * (err - s_prev_err)) / 10;
  s_output = clamp(s_output, -PID_OUT_LIMIT, PID_OUT_LIMIT);
  s_prev_err = err;
}

uint16_t telemetry_frame(char *out, uint16_t cap)
{
  uint16_t pos = 0U;

  if (out == 0 || cap < 8U)
  {
    return 0U;
  }

  pos = append_milli(out, pos, cap, s_target);
  if (pos == 0U || pos + 1U >= cap)
  {
    return 0U;
  }
  out[pos++] = ',';
  pos = append_milli(out, pos, cap, s_actual);
  if (pos + 1U >= cap)
  {
    return 0U;
  }
  out[pos++] = ',';
  pos = append_milli(out, pos, cap, s_output);
  return pos;
}

/* 解析 "SET kp=12.345" 形式的定点参数 */
err_t telemetry_command(const char *line)
{
  int32_t value = 0;
  int32_t sign = 1;
  uint8_t frac_digits = 0U;
  const char *p;

  if (line == 0)
  {
    return ERR_NULL;
  }
  if (strncmp(line, "SET kp=", 7) != 0)
  {
    return ERR_PARAM;
  }

  p = line + 7;
  if (*p == '-')
  {
    sign = -1;
    p++;
  }

  while (*p != '\0')
  {
    if (*p == '.')
    {
      frac_digits = 1U;
      p++;
      continue;
    }
    if (*p < '0' || *p > '9')
    {
      return ERR_PARAM;
    }
    value = value * 10 + (*p - '0');
    if (frac_digits > 0U)
    {
      frac_digits++;
    }
    p++;
  }

  if (frac_digits == 0U)
  {
    value *= PID_SCALE;
  }
  else
  {
    while (frac_digits < 4U)
    {
      value *= 10;
      frac_digits++;
    }
    value /= 10;
  }

  value *= sign;
  if (value < 0 || value > 100000)
  {
    return ERR_RANGE;
  }

  s_gain.kp = value;
  return ERR_OK;
}
