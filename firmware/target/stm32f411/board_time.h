#ifndef MKR_BOARD_TIME_H
#define MKR_BOARD_TIME_H
#include <stdint.h>
/*
 * TIM2 free-running at 1 µs. Marker pulse/gap deadlines use CC1.
 * CC1E stays off, so TIM2_CH1 is not driven. PA15 remains GPIO ISO_PWR_EN.
 * PA13/PA14 are not touched.
 *
 * The compare ISR calls cb. A 1 ms SysTick still services the same deadline
 * and can only end a pulse late. Width has not been measured on a scope.
 * Call only after SYSCLK is the 84 MHz PLL (timer kernel clock is SYSCLK
 * because APB1 is divided).
 */
void board_time_start(void (*on_deadline)(void *ctx), void *ctx);
void TIM2_IRQHandler(void);
#endif
