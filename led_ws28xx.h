#ifndef __LED_WS28XX_H__
#define __LED_WS28XX_H__

#include "struct_typedef.h"
#include "string.h"
#include "main.h"
#include "gpio.h"

// 通过此处的宏定义选择使用PWM或者是SPI
#define LED_WS28XX_USING_PWM
// #define LED_WS28XX_USING_SPI

#define ONE_PULSE (143) // 1 码计数个数
#define ZERO_PULSE (67) // 0 码计数个数
// RESET码的时间长度=TOTAL_DMABUFF_LEN*1.25us

#define LED_NUMS (256)    // led总个数
#define LED_DATA_LEN (24) // led需要24个字节表示

#define HALF_DMABUFF_LEN (16)                    // dma发送一次的灯珠个数
#define TOTAL_DMABUFF_LEN (HALF_DMABUFF_LEN * 2) // dma双缓冲区总长度（led个数为单位)

extern uint8_t g_targetlight_color[3];
extern uint8_t rgb_buff[LED_NUMS][3];

void ws28xx_init(void);
void ws28xx_send(void);
void ws28xx_set_node(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void ws28xx_set_node_all(uint8_t r, uint8_t g, uint8_t b);
void led_rgb_send_half_callback(void);
void led_rgb_send_full_callback(void);

#endif
