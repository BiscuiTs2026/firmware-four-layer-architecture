#include <stdint.h>

typedef struct
{
  uint32_t magic;
  uint32_t seq;
} snap_t;

volatile snap_t g_snap __attribute__((section(".snap"), used));
