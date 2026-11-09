#ifndef MKR_BOARD_CLOCK_H
#define MKR_BOARD_CLOCK_H
#include <stdbool.h>
#include <stdint.h>
/*
 * Switch SYSCLK to the HSE crystal PLL. Returns false on HSE, PLL or
 * switch timeout and leaves SYSCLK on HSI. HSEBYP stays clear.
 * Call only after board_gpio_init_safe().
 * board_clock_start programs PH0/PH1 (analog, no pull) before HSEON.
 */
bool board_clock_start(void);
/* After Stop, HSI is SYSCLK and HSEON/PLLON have been cleared in hardware.
 * A call that finds the crystal PLL already up is a no-op success.
 */
bool board_clock_restore_after_stop(void);
/* GPIOH clock on, PH0/PH1 analog, pulls cleared. HSE must still be off
 * for the GPIO mode to be the pin state (RM0383 §8.3.14).
 */
void board_hse_pins_prepare(void);
void board_delay_cycles(uint32_t cycles);
/* Milliseconds via mkr_cycles_from_ms(). Requests above MKR_DELAY_MS_MAX
 * saturate. Not a calibrated scope delay when DWT_CYCCNT is stopped. */
void board_delay_ms(uint32_t ms);
bool board_wait_set(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_cycles);
bool board_wait_clear(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_cycles);
#endif
