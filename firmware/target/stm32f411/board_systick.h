#ifndef MKR_BOARD_SYSTICK_H
#define MKR_BOARD_SYSTICK_H
#include <stdint.h>
/* 1 ms tick from HCLK after the PLL switch. IRQ priority is below USB. */
void board_systick_start(void (*tick)(void *ctx), void *ctx);
void SysTick_Handler(void);
#endif
