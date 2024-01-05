#ifndef WAVE_ANALYSIS_H_
#define WAVE_ANALYSIS_H_

#include "stm32f4xx.h"

typedef struct
{
    uint16_t raw;
    uint16_t raw_last;
    uint16_t raw_max;

    uint8_t detect_state;

    uint16_t trig_cnt;
    uint16_t trig_point;

    uint16_t fall_cnt;

    uint16_t total_point;

    uint16_t total_bullet;

    uint32_t time_start;
    uint32_t time_end;
    uint16_t time_total;
    /* data */
} wave_analysis_t;

extern wave_analysis_t wave_ana0;
extern wave_analysis_t wave_ana1;
extern wave_analysis_t wave_ana2;

void wave_analysis(wave_analysis_t *wave);
uint32_t micros(void);


#endif
