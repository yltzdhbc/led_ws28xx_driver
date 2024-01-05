#ifndef __LED_WS28XX_H__
#define __LED_WS28XX_H__

#include <stdlib.h>
#include "string.h"
#include "main.h"
#include "gpio.h"

// 通过此处的宏定义选择使用PWM或者是SPI
#define LED_WS28XX_USING_PWM
// #define LED_WS28XX_USING_SPI

#ifdef LED_WS28XX_USING_PWM
#include "tim.h"
#define PWM_HTIM htim2
#define PWM_HTIM_CHN TIM_CHANNEL_1
#define PWM_HTIM_INT_CHN HAL_TIM_ACTIVE_CHANNEL_1
#endif

#ifdef LED_WS28XX_USING_SPI
#include "spi.h"

#endif

#define ONE_PULSE (143) // 1 码计数个数
#define ZERO_PULSE (67) // 0 码计数个数
// RESET码的时间长度=TOTAL_DMABUFF_LEN*1.25us

#define LED_NUMS (256)    // led总个数
#define LED_DATA_LEN (24) // led需要24个字节表示

#define HALF_DMABUFF_LEN (16)                    // dma发送一次的灯珠个数
#define TOTAL_DMABUFF_LEN (HALF_DMABUFF_LEN * 2) // dma双缓冲区总长度（led个数为单位)

extern uint8_t rgb_buff[LED_NUMS][3];

void ws28xx_init(void);
void ws28xx_send(void);
void ws28xx_set_node(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
void ws28xx_set_node_all(uint8_t r, uint8_t g, uint8_t b);
void ws28xx_send_half_callback(void);
void ws28xx_send_full_callback(void);

#endif

#if 0

typedef struct
{
    TIM_HandleTypeDef *pwm_tim;
    uint32_t pwm_channel;

    uint16_t led_nums;

    uint16_t half_dmabuff_len;
    uint16_t total_dmabuff_len;

    uint8_t dma_busy_flag;
    uint8_t dma_send_nums;
    uint8_t *rgb_buff;
    uint16_t *dma_buff;

    // uint8_t rgb_buff[LED_NUMS][3];

    // void (*event_cb[BTN_EVENT_SUM])(agile_btn_t *btn); /**< 按键对象事件回调函数 */
} ws28xx_ctrl_t;

#endif
