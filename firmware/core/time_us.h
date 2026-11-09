#ifndef MKR_TIME_US_H
#define MKR_TIME_US_H
#include <stdint.h>
/*
 * Monotonic microseconds.
 * Target: TIM2 free-running at 1 MHz (board_time.c). That override is the
 * device clock. Host tests use the software counter below.
 * mkr_time_soft_* does not move TIM2.
 */
uint32_t mkr_hw_time_us(void);
/* Arm a TIM2 compare at deadline_us. 0-length is the caller's problem. */
void mkr_hw_arm_deadline(uint32_t deadline_us);
void mkr_hw_disarm_deadline(void);
void mkr_time_soft_set_us(uint32_t now_us);
void mkr_time_soft_advance_us(uint32_t delta_us);
#endif
