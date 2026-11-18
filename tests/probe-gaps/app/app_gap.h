#ifndef APP_GAP_H
#define APP_GAP_H

#include <stdint.h>

/* 旧版漏：extern 只扫 .c，头文件里的跨模块变量完全没查 */
extern uint32_t g_shared;

#endif
