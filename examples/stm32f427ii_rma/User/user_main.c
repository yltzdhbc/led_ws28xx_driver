#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

#include "wave_analysis.h"
#include "stm32f4xx.h"
#include "math.h"

#include "oled.h"
#include "wave_analysis.h"

#include "sw_i2c.h"
extern sw_i2c_t sw_i2c_stm32_f4;

int16_t charge12mm_brightness_compare = 100;

uint8_t key_handle_state[5] = {0};
uint32_t bullet_charge_timems = 200;

uint32_t adc1_dma_value_raw[4];

uint32_t adc2_dma_value_raw[2];

uint32_t sys_ms_last = 0;
uint32_t sys_ms_last1 = 0;

uint8_t record_point_flag = 0;
uint8_t record_point_finish = 0;
uint32_t record_point_cnt = 0;

uint16_t record_point_buf[1000] = {0};

// uint32_t temp_freq  = 200-1;

uint32_t temp_reload = 20000 - 1; // 500HZ
uint32_t temp_compare = 0;        // 500HZ

uint32_t temp_freq = 500;

uint32_t temp_ddd_time[3];

uint32_t record_us[3];
uint32_t last_record_us = 0;

extern uint16_t num_of_bullet;
extern uint16_t adc_value_max;
extern uint16_t number_of_point;
extern uint16_t time_of_wave;
extern uint16_t result_buffer[10];

uint8_t batt_P[10];

uint8_t check_flag = 0;
uint8_t read_flag = 0;

uint32_t calc_time = 0;
uint32_t calc_point = 0;

uint32_t current_micros[3] = {0};

uint32_t average_brig = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim12) // 10ms
    {

        temp_freq += 400;
        if (temp_freq >= 50000)
        {
            temp_freq = 500;
        }

        temp_reload = (uint32_t)(20000.0f * 500.0f / temp_freq);

        temp_compare = temp_reload / 2;

        (&htim8, temp_reload - 1);
        __HAL_TIM_SET_COMPARE(&htim8, TIM_CHANNEL_1, temp_compare - 1);

        oled_adc_button_handler();
    }
    else if (htim == &htim13)
    {
    }
    else if (htim == &htim14)
    {
        HAL_GPIO_WritePin(CHARGE12MM_GPIO_Port, CHARGE12MM_Pin, GPIO_PIN_RESET);
        __HAL_TIM_DISABLE(&htim14);

        record_point_cnt = 0;
        record_point_flag = 1;
        temp_ddd_time[0] = HAL_GetTick();

        last_record_us = micros();
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{

    if (hadc == &hadc1)
    {
#if STATIC_TEST_MODEL
        current_micros[0] = micros();
        current_micros[2] = current_micros[0] - current_micros[1];
        current_micros[1] = micros();
        if (micros() - last_record_us > 1000)
        {
            last_record_us = micros();

            if (record_point_flag == 1)
            {
                record_point_buf[record_point_cnt] = adc1_dma_value_raw[0];
                record_point_cnt++;
                if (record_point_cnt >= 1000)
                // if (adc1_dma_value_raw[0] < 100)
                {
                    temp_ddd_time[1] = HAL_GetTick();
                    temp_ddd_time[2] = temp_ddd_time[1] - temp_ddd_time[0];
                    calc_point = record_point_cnt;
                    // calc_time = calc_point * 22;
                    record_point_flag = 0;
                    record_point_cnt = 0;
                    record_point_finish = 1;
                }
            }
        }
#else
        wave_ana0.raw = adc1_dma_value_raw[0];
        wave_analysis(&wave_ana0);
        wave_ana1.raw = adc1_dma_value_raw[1];
        wave_analysis(&wave_ana1);
        wave_ana2.raw = adc1_dma_value_raw[2];
        wave_analysis(&wave_ana2);
#endif
    }
}

void user_setup(void)
{
    oled_init();

    HAL_GPIO_WritePin(GPIOH, PWR1_CTRL_Pin | PWR2_CTRL_Pin | PWR3_CTRL_Pin | PWR4_CTRL_Pin, GPIO_PIN_SET);

    // HAL_TIM_Base_Start_IT(&htim12); // 10ms
    // HAL_TIM_Base_Start_IT(&htim13);
    // HAL_TIM_Base_Start_IT(&htim14);

    // HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);
    // __HAL_TIM_SET_PRESCALER(&htim8, 18 - 1);

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    __HAL_TIM_SET_PRESCALER(&htim4, 90 - 1);            // 1m
    __HAL_TIM_SET_AUTORELOAD(&htim4, 20000 - 1);        // 0-20000   50HZ  20ms 。 1ms-对应填充值 - 1000
    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 2000); // 2ms 对应了 2000

    HAL_Delay(2000);

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1000); // 2ms 对应了 2000
    HAL_Delay(2000);

    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1500); // 2ms 对应了 2000
    HAL_Delay(1000);

    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_13, GPIO_PIN_SET);

    // __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1600); // 2ms 对应了 2000
    // HAL_Delay(1000);
    // __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1700); // 2ms 对应了 2000
    // HAL_Delay(1000);
    // __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, 1800); // 2ms 对应了 2000
    // HAL_Delay(1000);

    // HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);
    // __HAL_TIM_SET_PRESCALER(&htim5, 18 - 1);
    // __HAL_TIM_SET_COMPARE(&htim5, TIM_CHANNEL_4, charge12mm_brightness_compare);

    //  __HAL_TIM_SET_AUTORELOAD(&htim9, temp_freq);
    //  __HAL_TIM_SET_COMPARE(&htim9, TIM_CHANNEL_1, temp_freq/2);

    // HAL_ADC_Start_DMA(&hadc1, adc1_dma_value_raw, 4);
    // HAL_ADC_Start_DMA(&hadc2, adc2_dma_value_raw, 1);

    //  __HAL_I2C_ENABLE(&hi2c2);
    //
    //  __HAL_I2C_RESET_HANDLE_STATE(&hi2c2);
    // __HAL_RCC_I2C2_FORCE_RESET();
    // __HAL_RCC_I2C2_RELEASE_RESET();

    // SW_I2C_initial(&sw_i2c_stm32_f4);
}

uint16_t temp = 1500;

void user_loop(void)
{

    if (HAL_GetTick() - sys_ms_last1 > 1000)
    {
        sys_ms_last1 = HAL_GetTick();
        temp += 100;
        if (temp >= 2000)
            temp = 1100;

        __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, temp);
    }

    if (HAL_GetTick() - sys_ms_last > 100)
    {
        sys_ms_last = HAL_GetTick();

        // HAL_I2C_Master_Receive(&hi2c2, 0X0B, batt_P, 2, 5);
        // HAL_I2C_Mem_Read(&hi2c2, 0X0B, 0X09, I2C_MEMADD_SIZE_16BIT, batt_P, 2, 5000);

        // check_flag = SW_I2C_Check_SlaveAddr(&sw_i2c_stm32_f4, 0X0B);
        // read_flag =  SW_I2C_Read_16addr(&sw_i2c_stm32_f4, 0X0B, 0x09, batt_P, 1);

        HAL_GPIO_TogglePin(LED_R_GPIO_Port, LED_R_Pin);
        HAL_GPIO_TogglePin(LED_G_GPIO_Port, LED_G_Pin);

        oled_clear(Pen_Clear);

#if STATIC_TEST_MODEL

        if (record_point_flag == 0)
        {
            oled_printf(0, 1, "complete.");
        }
        else
        {
            oled_printf(0, 1, "measuring..");
        }

        oled_printf(2, 1, "ms: %d", temp_ddd_time[2]);
        oled_printf(0, 12, "time:%d", bullet_charge_timems / 10);
        oled_printf(1, 12, "butt:%d", oled_adc_button_get());
        oled_printf(2, 12, "rawb:%d", adc2_dma_value_raw[0]);
        oled_printf(3, 12, "rawL:%d", adc1_dma_value_raw[0]);
#else
        oled_printf(0, 1, "raw0 : %d", wave_ana0.raw);
        // oled_printf(1, 1, "raw1 : %d", wave_ana1.raw);
        // oled_printf(2, 1, "raw2 : %d", wave_ana2.raw);

        // oled_printf(3, 1, "raw3 : %d", adc1_dma_value_raw[3]);

        // oled_printf(1, 1, "max   : %d", adc_value_max);
        oled_printf(2, 1, "cnt   : %d", wave_ana0.total_bullet);
        oled_printf(3, 1, "point : %d", wave_ana0.total_point);
        oled_printf(4, 1, "time  : %.1f", wave_ana0.time_total / 1000.0f);

        // oled_drawline(80, 1, 80, 64 - 1, Pen_Write);

        // oled_printf(0, 15, "%d", result_buffer[0]);
        // oled_printf(1, 15, "%d", result_buffer[1]);
        // oled_printf(2, 15, "%d", result_buffer[2]);
        // oled_printf(3, 15, "%d", result_buffer[3]);
        // oled_printf(4, 15, "%d", result_buffer[4]);

        oled_printf(0, 13, "m0:%d", wave_ana0.raw_max);
        oled_printf(1, 13, "m1:%d", wave_ana1.raw_max);
        oled_printf(2, 13, "m2:%d", wave_ana2.raw_max);

        average_brig = (wave_ana0.raw_max + wave_ana0.raw_max + wave_ana0.raw_max) / 3;
        oled_printf(3, 13, "ma:%d", average_brig);

        oled_printf(4, 13, "br:%d", charge12mm_brightness_compare / 10);

        // uint8_t ddd = 5;
        // oled_drawline(110, 10 - ddd, 115, 10 - ddd, Pen_Write);
        // oled_drawline(110, 10 - ddd, 112, 8 - ddd, Pen_Write);
        // oled_drawline(110, 10 - ddd, 112, 12 - ddd, Pen_Write);
        // oled_drawline(127, 0, 127, 63 - 0, Pen_Write);

#endif

        oled_refresh_gram();
    }

    if (OLED_KEY_MIDDLE == oled_adc_button_get() && key_handle_state[OLED_KEY_MIDDLE] == 0)
    {
        key_handle_state[OLED_KEY_MIDDLE] = 1;
    }
    else if (OLED_KEY_MIDDLE != oled_adc_button_get() && key_handle_state[OLED_KEY_MIDDLE] == 1)
    {
        key_handle_state[OLED_KEY_MIDDLE] = 0;
        __HAL_TIM_SET_AUTORELOAD(&htim14, bullet_charge_timems);
        __HAL_TIM_SET_COUNTER(&htim14, 0);
        __HAL_TIM_ENABLE(&htim14);

        HAL_GPIO_WritePin(CHARGE12MM_GPIO_Port, CHARGE12MM_Pin, GPIO_PIN_SET);

        temp_ddd_time[2] = 0;
    }

    if (OLED_KEY_UP == oled_adc_button_get() && key_handle_state[OLED_KEY_UP] == 0)
    {
        key_handle_state[OLED_KEY_UP] = 1;
    }
    else if (OLED_KEY_UP != oled_adc_button_get() && key_handle_state[OLED_KEY_UP] == 1)
    {
        key_handle_state[OLED_KEY_UP] = 0;
        bullet_charge_timems += 100;
        charge12mm_brightness_compare += 100;
    }

    if (OLED_KEY_DOWN == oled_adc_button_get() && key_handle_state[OLED_KEY_DOWN] == 0)
    {
        key_handle_state[OLED_KEY_DOWN] = 1;
    }
    else if (OLED_KEY_DOWN != oled_adc_button_get() && key_handle_state[OLED_KEY_DOWN] == 1)
    {
        key_handle_state[OLED_KEY_DOWN] = 0;
        bullet_charge_timems -= 100;
        charge12mm_brightness_compare -= 100;
    }

    if (OLED_KEY_LEFT == oled_adc_button_get() && key_handle_state[OLED_KEY_LEFT] == 0)
    {
        key_handle_state[OLED_KEY_LEFT] = 1;
    }
    else if (OLED_KEY_LEFT != oled_adc_button_get() && key_handle_state[OLED_KEY_LEFT] == 1)
    {
        key_handle_state[OLED_KEY_LEFT] = 0;
        bullet_charge_timems -= 10;
        charge12mm_brightness_compare -= 10;
    }

    if (OLED_KEY_RIGHT == oled_adc_button_get() && key_handle_state[OLED_KEY_RIGHT] == 0)
    {
        key_handle_state[OLED_KEY_RIGHT] = 1;
    }
    else if (OLED_KEY_RIGHT != oled_adc_button_get() && key_handle_state[OLED_KEY_RIGHT] == 1)
    {
        key_handle_state[OLED_KEY_RIGHT] = 0;
        bullet_charge_timems += 10;
        charge12mm_brightness_compare += 10;
    }

    if (charge12mm_brightness_compare < 10)
    {
        charge12mm_brightness_compare = 10;
    }
    else if (charge12mm_brightness_compare > 1000)
    {
        charge12mm_brightness_compare = 1000;
    }

    // __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, charge12mm_brightness_compare);

    // 50HZ  = 20ms
    //
    // 1HZ   = 1000MS
    if (bullet_charge_timems < 100)
    {
        bullet_charge_timems = 100;
    }
    else if (bullet_charge_timems > 10000)
    {
        bullet_charge_timems = 10000;
    }

    //    if (1U == HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin))
    //    {
    //      HAL_Delay(10);
    //      if (1U == HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin))
    //      {
    //        while (1U == HAL_GPIO_ReadPin(USER_KEY_GPIO_Port, USER_KEY_Pin))
    //          ;
    //        start_charge_ms = HAL_GetTick();

    //        __HAL_TIM_SET_AUTORELOAD(&htim14, 5000);
    //        __HAL_TIM_SET_COUNTER(&htim14, 0);
    //        __HAL_TIM_ENABLE(&htim14);

    //        HAL_GPIO_WritePin(CHARGE12MM_GPIO_Port, CHARGE12MM_Pin, GPIO_PIN_SET);
    //      }
    //    }
}
