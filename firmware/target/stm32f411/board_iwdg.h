#ifndef MKR_BOARD_IWDG_H
#define MKR_BOARD_IWDG_H
#include <stdbool.h>
/*
 * Arm the IWDG only after bring-up has succeeded. A kick before that is ignored
 * so the 0x5555 programming sequence cannot be broken by the 1 ms tick.
 * On timeout the hardware resets into Reset_Handler; main calls
 * board_gpio_init_safe() before anything else. This is a software policy.
 * The window has not been measured on a board.
 */
bool board_iwdg_start(void);
void board_iwdg_kick(void);
/* Clear the RTC wakeup flag and EXTI line 22 so Stop entry is not blocked. */
void board_rtc_acknowledge_wakeup(void);
void RTC_WKUP_IRQHandler(void);
#endif
