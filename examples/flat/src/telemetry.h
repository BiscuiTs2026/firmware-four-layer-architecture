#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "types.h"

#define TELEM_CMD_MAX   24U
#define TELEM_FRAME_MAX 40U

/* 采样节拍由调用方按 BOARD_SAMPLE_MS 推进 */
void telemetry_init(void);
void telemetry_sample(void);

/*
 * 生成一帧 CSV：target,actual,output（3 位小数，无表头）。
 * 返回写入长度，0 表示缓冲区不足；可重入：否（内部无状态，仅依赖快照）。
 */
uint16_t telemetry_frame(char *out, uint16_t cap);

/* 处理一行指令：SET kp=1.000 / GET。成功返回 ERR_OK */
err_t telemetry_command(const char *line);

#endif
