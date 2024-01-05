#include "wave_analysis.h"
#include "stm32f4xx.h"
#include "math.h"

#define RISE_INCREMENTAL 2
#define RISE_THRESHOLD 200
#define RISE_TIMES 5

#define FALL_THRESHOLD 200
#define FALL_TIMES 5

uint16_t result_buffer[10];

// uint16_t wave->fall_cnt = 0;
// uint16_t wave->trig_cnt = 0;
// uint16_t wave->trig_point = 0;
// uint16_t wave->raw_max = 0;
// uint16_t wave->total_bullet = 0;
// uint32_t temp_time[3] = {0};
// uint16_t wave->total_point = 0;
// uint16_t wave->total_time = 0;
// uint8_t wave->detect_state = 0;

wave_analysis_t wave_ana0;
wave_analysis_t wave_ana1;
wave_analysis_t wave_ana2;

void wave_analysis(wave_analysis_t *wave)
{
    switch (wave->detect_state)
    {
    case 0:
    {
        // 触发条件：连续上升，每次采样上升值大于2，持续5次
        if ((wave->raw - wave->raw_last) > RISE_INCREMENTAL && wave->raw > RISE_THRESHOLD)
        {
            wave->trig_cnt++;
        }
        else
        {
            wave->trig_cnt = 0;
        }
        if (wave->trig_cnt > RISE_TIMES)
        {
            wave->trig_cnt = 0;
            wave->time_start = micros();
            wave->raw_max = 0;
            wave->detect_state = 1;
        }
        break;
    }
    case 1:
    {
        wave->trig_point++;

        // 找到本次窗口的最大值
        if (wave->raw > wave->raw_max)
        {
            wave->raw_max = wave->raw;
        }

        // 结束条件，采样值小于50，连续10次
        if ((wave->raw) < FALL_THRESHOLD)
        {
            wave->fall_cnt++;
        }
        else
        {
            wave->fall_cnt = 0;
        }
        if (wave->fall_cnt > FALL_TIMES)
        {
            wave->fall_cnt = 0;
            wave->detect_state = 2;
        }
        break;
    }
    case 2:
    {
        wave->time_end = micros();
        wave->time_total = wave->time_end - wave->time_start;

        wave->total_point = wave->trig_point;
        // wave->total_time = wave->total_point * 42;
        // wave->total_time = wave->time_total;

        wave->trig_point = 0;
        wave->total_bullet++;

        for (int i = 4; i >= 1; i--)
        {
            result_buffer[i] = result_buffer[i - 1];
        }
        result_buffer[0] = wave->raw_max;

        wave->detect_state = 0;
        break;
    }
    default:
        break;
    }

    wave->raw_last = wave->raw;
}

__STATIC_INLINE uint32_t GXT_SYSTICK_IsActiveCounterFlag(void)
{
    return ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == (SysTick_CTRL_COUNTFLAG_Msk));
}

static uint32_t getCurrentMicros(void)
{
    /* Ensure COUNTFLAG is reset by reading SysTick control and status register */
    GXT_SYSTICK_IsActiveCounterFlag();
    uint32_t m = HAL_GetTick();
    const uint32_t tms = SysTick->LOAD + 1;
    __IO uint32_t u = tms - SysTick->VAL;
    if (GXT_SYSTICK_IsActiveCounterFlag())
    {
        m = HAL_GetTick();
        u = tms - SysTick->VAL;
    }
    return (m * 1000 + (u * 1000) / tms);
}

// 获取系统时间，单位us
uint32_t micros(void)
{
    return getCurrentMicros();
}

// if (trigger_flag == 1)
// {
//     wave->trig_point++;

//     if (wave->raw > wave->raw_max)
//     {
//         wave->raw_max = wave->raw;
//     }

//     // 40us 一次，100ms的窗口 100*1000/40 = 2500个点
//     //            if (wave->trig_point > 2500 * 3)

//     if ((wave->raw) < 50)
//     {
//         wave->fall_cnt++;
//     }
//     else
//     {
//         wave->fall_cnt = 0;
//     }

//     if (wave->fall_cnt > 10)
//     {
//         falling_flag = 1;
//     }

//     // if ((wave->time_end > 1000 * 300) || falling_flag == 1)
//     if (falling_flag == 1)
//     {
//         wave->time_start = sys_us_get();
//         wave->time_end = wave->time_start - wave->time_total;

//         wave->total_point = wave->trig_point;
//         wave->total_time = wave->total_point * 42;

//         falling_flag = 0;
//         wave->trig_point = 0;
//         trigger_flag = 0;
//         oled_refresh_flag = 1;
//         wave->total_bullet++;

//         for (int i = 4; i >= 1; i--)
//         {
//             result_buffer[i] = result_buffer[i - 1];
//         }
//         result_buffer[0] = wave->raw_max;
//     }

//     // adc_record_buf[adc_record_idx] = wave->raw;
//     // adc_record_idx++;
//     // if (adc_record_idx >= 2000)
//     // {
//     //     adc_record_idx = 0;
//     //     trigger_flag = 0;
//     //     oled_refresh_flag = 1;
//     // }
// }

// // adc_trans_cnt++;
