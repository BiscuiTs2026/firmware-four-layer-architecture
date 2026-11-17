#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* 板级常量集中在这里，业务代码不写裸数字 */
#define BOARD_MCU_HZ        8000000U
#define BOARD_TICK_HZ       1000U
#define BOARD_UART_BAUD     9600U
#define BOARD_SAMPLE_MS     10U
#define BOARD_FRAME_EVERY   5U

#define BOARD_LED_PORT_IDX  2U   /* GPIOC */
#define BOARD_LED_PIN       13U

#endif
