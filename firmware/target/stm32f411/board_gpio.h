#ifndef MKR_BOARD_GPIO_H
#define MKR_BOARD_GPIO_H
#include <stdbool.h>
#include <stdint.h>
#include "board_pins.h"
void board_gpio_init_safe(void);
void board_marker_write(uint8_t code, bool ready);
void board_iso_power_set(bool enabled);
/* Clear PB0-PB8 first, then drive PA15 low. Safe to call from fault handlers. */
void board_gpio_force_safe_off(void);
#endif
