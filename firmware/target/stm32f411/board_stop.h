#ifndef MKR_BOARD_STOP_H
#define MKR_BOARD_STOP_H
#include <stdbool.h>
/*
 * Stop, low-power regulator, low-voltage (RM0383 PWR_CR):
 * LPDS and LPLVDS set, PDDS clear, FPDS clear. Flash stays powered.
 * This is not Flash deep power-down. SRAM and registers are kept,
 * including the USB device. Standby is not used.
 * Wake sources are USB OTG FS EXTI line 18 and the RTC wakeup (EXTI line 22).
 * Suspend current is not measured by entering this mode.
 */
void board_stop_enter(void);
void board_stop_clear_sleepdeep(void);
bool mkr_wakeup_vectors_installed(void);
void OTG_FS_WKUP_IRQHandler(void);
#endif
