/**
 * @file task_distance.h
 * @brief VL53L0X distance sensor interface.
 */

#ifndef INC_TASK_DISTANCE_H_
#define INC_TASK_DISTANCE_H_

#include "main.h"
#include "vl53l0x_api.h"
#include <stdint.h>
#include "usb_print.h"

typedef void (*state_fcn_t)(void);
typedef void (*task_distance_state_func_t)(void);
typedef struct
{
    int32_t state;
    uint32_t runs;
    uint32_t num_states;
    task_distance_state_func_t state_list[3];
} task_distance_cfg;

typedef struct
{
    uint16_t distance_mm;
    uint8_t range_status;
    int16_t error;
} TOF_Measurement_t;

extern task_distance_cfg task_distance;

void task_distance_initialize(void);
void task_distance_run(void);
void task_distance_state_0(void);
void task_distance_state_1(void);
void task_distance_state_2(void);

void TOF_I2C_Scan_With_Both_On(void);
int16_t TOF_InitTwo(void);
void TOF_ReadTwo(TOF_Measurement_t *tof1,
                 TOF_Measurement_t *tof2);

#endif /* INC_TASK_DISTANCE_H_ */
