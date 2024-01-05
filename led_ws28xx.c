#include "led_ws28xx.h"

// dma发送标志位
uint8_t ws28xx_dma_busy_flag = 0;
// dma发送灯珠的数量
uint16_t ws28xx_dma_send_nums = 0;
// 所有灯珠节点的RGB值
uint8_t rgb_buff[LED_NUMS][3] = {0};

#ifdef LED_WS28XX_USING_PWM
// dma缓冲区
uint16_t rgb_dma_buff[LED_DATA_LEN * TOTAL_DMABUFF_LEN] = {0};
#endif

#ifdef LED_WS28XX_USING_SPI
// dma缓冲区
uint8_t rgb_dma_buff[LED_DATA_LEN * TOTAL_DMABUFF_LEN] = {0};
#endif

/*-------------------------- 硬件抽象层 PWM SPI DMA发送一半 && DMA发送完成 --------------------------*/
#ifdef LED_WS28XX_USING_PWM
/** @brief 将颜色RGB数组转换为单总线的编码存放到DMA缓冲数组中
 */
static inline void ws28xx_write_buff(uint16_t *buff, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 8; i++)
    {
        buff[i] = (g << i) & (0x80) ? ONE_PULSE : ZERO_PULSE;
        buff[i + 8] = (r << i) & (0x80) ? ONE_PULSE : ZERO_PULSE;
        buff[i + 16] = (b << i) & (0x80) ? ONE_PULSE : ZERO_PULSE;
    }
}

static inline void ws28xx_dma_start(void)
{
    PWM_HTIM.State = HAL_TIM_STATE_READY;
    HAL_TIM_PWM_Start_DMA(&PWM_HTIM, PWM_HTIM_CHN, (uint32_t *)rgb_dma_buff, (LED_DATA_LEN * TOTAL_DMABUFF_LEN));

}

static inline void ws28xx_dma_stop(void)
{
    HAL_TIM_PWM_Stop_DMA(&PWM_HTIM, PWM_HTIM_CHN);
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &PWM_HTIM)
    {
        if (htim->Channel == PWM_HTIM_INT_CHN)
        { 
            ws28xx_send_half_callback();
        }
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &PWM_HTIM)
    {
        if (htim->Channel == PWM_HTIM_INT_CHN)
        {
            ws28xx_send_full_callback();
        }
    }
}

#endif

#ifdef LED_WS28XX_USING_SPI
/** @brief 将颜色RGB数组转换为单总线的编码存放到DMA缓冲数组中
 */
static inline void ws28xx_write_buff(uint8_t *buff, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t grb = 0;
    uint8_t i = 0;

    grb = g << 24 | r << 16 | b << 8;
    i = 24;

    while (i--)
    {
        // bit 1:1111 1000
        if (grb & 0x80000000)
        {
            *buff = 0xf8; // f8 fc fe
        }
        // bit 0:1100 0000
        else
        {
            *buff++ = 0xc0; // 80 c0 e0
        }
        grb <<= 1;
    }
}

static inline void ws28xx_dma_start(void)
{
    HAL_SPI_Transmit_DMA(&hspi2, (uint8_t *)rgb_dma_buff, (LED_DATA_LEN * TOTAL_DMABUFF_LEN));
}

static inline void ws28xx_dma_stop(void)
{
    HAL_SPI_DMAStop(&hspi2);
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2)
    {
        ws28xx_send_half_callback();
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2)
    {
        ws28xx_send_full_callback();
    }
}
#endif

/*-------------------------- 用户逻辑层 硬件无关 --------------------------*/
/** @brief 初始化
 *
 */
void ws28xx_init(void)
{
    ws28xx_set_node_all(0, 0, 0);
    ws28xx_send();
}

/** @brief 设置单个节点缓存区的颜色
 *
 */
void ws28xx_set_node(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    rgb_buff[index][0] = r;
    rgb_buff[index][1] = g;
    rgb_buff[index][2] = b;
}

/** @brief 设置所有节点缓存区的颜色
 *
 */
void ws28xx_set_node_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < LED_NUMS; i++)
    {
        ws28xx_set_node(i, r, g, b);
    }
}

/** @brief 开启DMA发送流程
 *
 */
void ws28xx_send(void)
{
    if (ws28xx_dma_busy_flag == 1)
        return;

    memset(rgb_dma_buff, 0x00, sizeof(rgb_dma_buff));

    ws28xx_dma_start();
}

/** @brief DMA发送一半中断回调
 *  @note  双缓冲发送机制，发送一半后将后面的数据填充到前一半
 */
void ws28xx_send_half_callback(void)
{
    for (int i = 0; i < HALF_DMABUFF_LEN; i++)
    {
        ws28xx_write_buff(&rgb_dma_buff[LED_DATA_LEN * i],       //
                          rgb_buff[ws28xx_dma_send_nums + i][0], //
                          rgb_buff[ws28xx_dma_send_nums + i][1], //
                          rgb_buff[ws28xx_dma_send_nums + i][2]);
    }
    ws28xx_dma_send_nums += HALF_DMABUFF_LEN;
}

/** @brief DMA发送完成中断回调
 *  @note  双缓冲发送机制，发送完成后将后面的数据填充到后一半
 */
void ws28xx_send_full_callback(void)
{
    if (ws28xx_dma_send_nums >= LED_NUMS)
    {
        ws28xx_dma_stop();
        ws28xx_dma_send_nums = 0;
        ws28xx_dma_busy_flag = 0;
        return;
    }
    for (int i = 0; i < HALF_DMABUFF_LEN; i++)
    {
        ws28xx_write_buff(&rgb_dma_buff[LED_DATA_LEN * (i + HALF_DMABUFF_LEN)], //
                          rgb_buff[ws28xx_dma_send_nums + i][0],                //
                          rgb_buff[ws28xx_dma_send_nums + i][1],                //
                          rgb_buff[ws28xx_dma_send_nums + i][2]);
    }
    ws28xx_dma_send_nums += HALF_DMABUFF_LEN;
}

#if 0

#include "led_ws28xx.h"

// 所有灯珠节点的RGB值
uint8_t rgb_buff_0[LED_NUMS][3] = {0};
uint16_t dma_buff_0[LED_DATA_LEN * TOTAL_DMABUFF_LEN] = {0};

ws28xx_ctrl_t ws28xx[1];

// dma发送标志位
uint8_t ws28xx_dma_busy_flag = 0;
// dma发送灯珠的数量
uint16_t ws28xx_dma_send_nums = 0;
// 所有灯珠节点的RGB值
uint8_t rgb_buff[LED_NUMS][3] = {0};

#ifdef LED_WS28XX_USING_PWM
#include "tim.h"
// extern TIM_HandleTypeDef htim1;
// dma缓冲区
uint16_t rgb_dma_buff[LED_DATA_LEN * TOTAL_DMABUFF_LEN] = {0};
#endif

#ifdef LED_WS28XX_USING_SPI
#include "spi.h"
extern SPI_HandleTypeDef hspi2;
// dma缓冲区
uint8_t rgb_dma_buff[LED_DATA_LEN * TOTAL_DMABUFF_LEN] = {0};
#endif

/*-------------------------- 硬件抽象层 PWM SPI DMA发送一半 && DMA发送完成 --------------------------*/
#ifdef LED_WS28XX_USING_PWM
/** @brief 将颜色RGB数组转换为单总线的编码存放到DMA缓冲数组中
 */
static inline void ws28xx_write_buff(uint16_t *buff, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 8; i++)
    {
        buff[i] = (g << i) & (0x80) ? ONE_PULSE : ZERO_PULSE;
        buff[i + 8] = (r << i) & (0x80) ? ONE_PULSE : ZERO_PULSE;
        buff[i + 16] = (b << i) & (0x80) ? ONE_PULSE : ZERO_PULSE;
    }
}

static inline void ws28xx_dma_start(uint8_t ch)
{
    HAL_TIM_PWM_Start_DMA(ws28xx[ch].pwm_tim,             //
                          ws28xx[ch].pwm_channel,         //
                          (uint32_t *)ws28xx[ch].dma_buff, //
                          (LED_DATA_LEN * ws28xx[ch].total_dmabuff_len));

    // HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)rgb_dma_buff, (LED_DATA_LEN * TOTAL_DMABUFF_LEN));
}

static inline void ws28xx_dma_stop(uint8_t ch)
{
    HAL_TIM_PWM_Stop_DMA(ws28xx[ch].pwm_tim, //
                         ws28xx[ch].pwm_channel);
    // HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            ws28xx_send_half_callback(0);
        }
    }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1)
    {
        if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
        {
            ws28xx_send_full_callback(0);
        }
    }
}

#endif

#ifdef LED_WS28XX_USING_SPI
/** @brief 将颜色RGB数组转换为单总线的编码存放到DMA缓冲数组中
 */
static inline void ws28xx_write_buff(uint8_t *buff, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t grb = 0;
    uint8_t i = 0;

    grb = g << 24 | r << 16 | b << 8;
    i = 24;

    while (i--)
    {
        // bit 1:1111 1000
        if (grb & 0x80000000)
        {
            *buff = 0xf8; // f8 fc fe
        }
        // bit 0:1100 0000
        else
        {
            *buff++ = 0xc0; // 80 c0 e0
        }
        grb <<= 1;
    }
}

static inline void ws28xx_dma_start(void)
{
    HAL_SPI_Transmit_DMA(&hspi2, (uint8_t *)rgb_dma_buff, (LED_DATA_LEN * TOTAL_DMABUFF_LEN));
}

static inline void ws28xx_dma_stop(void)
{
    HAL_SPI_DMAStop(&hspi2);
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2)
    {
        ws28xx_send_half_callback();
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2)
    {
        ws28xx_send_full_callback();
    }
}
#endif

/*-------------------------- 用户逻辑层 硬件无关 --------------------------*/
/** @brief 初始化
 *
 */
void ws28xx_init(void)
{
    ws28xx[0].rgb_buff = &rgb_buff_0;
    ws28xx[0].dma_buff = &dma_buff_0[0];

    ws28xx[0].pwm_tim = &htim4;
    ws28xx[0].pwm_channel = TIM_CHANNEL_1;

    ws28xx[0].half_dmabuff_len = 16;
    ws28xx[0].total_dmabuff_len = ws28xx[0].half_dmabuff_len * 2;

    ws28xx_set_node_all(0, 0, 0);
    ws28xx_send(0);
}

/** @brief 设置单个节点缓存区的颜色
 *
 */
void ws28xx_set_node(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    rgb_buff[index][0] = r;
    rgb_buff[index][1] = g;
    rgb_buff[index][2] = b;
}

/** @brief 设置所有节点缓存区的颜色
 *
 */
void ws28xx_set_node_all(uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < LED_NUMS; i++)
    {
        ws28xx_set_node(i, r, g, b);
    }
}

/** @brief 开启DMA发送流程
 *
 */
void ws28xx_send(uint8_t ch)
{
    if (ws28xx_dma_busy_flag == 1)
        return;

    memset(rgb_dma_buff, 0x00, sizeof(rgb_dma_buff));

    ws28xx_dma_start(ch);
}

/** @brief DMA发送一半中断回调
 *  @note  双缓冲发送机制，发送一半后将后面的数据填充到前一半
 */
void ws28xx_send_half_callback(uint8_t ch)
{
    for (int i = 0; i < HALF_DMABUFF_LEN; i++)
    {
        ws28xx_write_buff(&ws28xx[ch].dma_buff[LED_DATA_LEN * i],               //
                          ws28xx[ch].rgb_buff[ws28xx[ch].dma_send_nums + i][0], //
                          ws28xx[ch].rgb_buff[ws28xx[ch].dma_send_nums + i][1], //
                          ws28xx[ch].rgb_buff[ws28xx[ch].dma_send_nums + i][2]);
        // ws28xx_write_buff(&rgb_dma_buff[LED_DATA_LEN * i],       //
        //                   rgb_buff[ws28xx_dma_send_nums + i][0], //
        //                   rgb_buff[ws28xx_dma_send_nums + i][1], //
        //                   rgb_buff[ws28xx_dma_send_nums + i][2]);
    }
    ws28xx[ch].dma_send_nums += HALF_DMABUFF_LEN;
}

/** @brief DMA发送完成中断回调
 *  @note  双缓冲发送机制，发送完成后将后面的数据填充到后一半
 */
void ws28xx_send_full_callback(uint8_t ch)
{
    if (ws28xx[ch].dma_send_nums >= LED_NUMS)
    {
        ws28xx_dma_stop(ch);
        // HAL_GPIO_WritePin(GPIOI, GPIO_PIN_7, GPIO_PIN_RESET);
        ws28xx[ch].dma_send_nums = 0;
        ws28xx[ch].dma_busy_flag = 0;
        return;
    }
    for (int i = 0; i < HALF_DMABUFF_LEN; i++)
    {
        ws28xx_write_buff(&ws28xx[ch].dma_buff[LED_DATA_LEN * (i + HALF_DMABUFF_LEN)], //
                          ws28xx[ch].rgb_buff[ws28xx[ch].dma_send_nums + i][0],        //
                          ws28xx[ch].rgb_buff[ws28xx[ch].dma_send_nums + i][1],        //
                          ws28xx[ch].rgb_buff[ws28xx[ch].dma_send_nums + i][2]);
        // ws28xx_write_buff(&rgb_dma_buff[LED_DATA_LEN * (i + HALF_DMABUFF_LEN)], //
        //                   rgb_buff[ws28xx_dma_send_nums + i][0],                //
        //                   rgb_buff[ws28xx_dma_send_nums + i][1],                //
        //                   rgb_buff[ws28xx_dma_send_nums + i][2]);
    }
    ws28xx[ch].dma_send_nums += HALF_DMABUFF_LEN;
}

#endif
