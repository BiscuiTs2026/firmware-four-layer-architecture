#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>

/* 统一错误码：全工程只认这一组 */
typedef enum {
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

typedef uint8_t  msg_id_t;
typedef uint8_t  msg_src_t;

#define MSG_PAYLOAD_MAX 32U

/* 消息统一结构：禁止各模块自造数据包 */
typedef struct {
    msg_id_t  id;
    msg_src_t src;
    uint32_t  tick;
    uint16_t  len;
    uint8_t   payload[MSG_PAYLOAD_MAX];
} message_t;

/* msg_id 按模块分段，新增先登记再用 */
#define MSG_ID_BSP_BASE   0x00U
#define MSG_ID_COMP_BASE  0x10U
#define MSG_ID_APP_BASE   0x30U

#define MSG_SYS_RUN       (MSG_ID_BSP_BASE + 0x01U)
#define MSG_LED_SET       (MSG_ID_COMP_BASE + 0x01U)

#endif
