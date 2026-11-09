#ifndef MKR_STOP_PLAN_H
#define MKR_STOP_PLAN_H
/*
 * Stop mode programmed by board_stop_enter().
 *
 * RM0383 PWR_CR (STM32F411):
 *   LPDS   bit 0  low-power regulator in Stop when set
 *   PDDS   bit 1  0 = Stop, 1 = Standby
 *   FPDS   bit 9  Flash power-down in Stop when set
 *   LPLVDS bit 10 low-power regulator low-voltage when set with LPDS
 *   MRLVDS bit 11 main regulator low-voltage; not used with LPDS
 *
 * This image selects Stop with the low-power regulator in low-voltage
 * mode and leaves Flash powered: LPDS=1, LPLVDS=1, PDDS=0, FPDS=0.
 * That is not the Flash deep-power-down row. SRAM and peripheral
 * registers, including the USB device, stay powered. Standby is not used.
 *
 * DS10314's wakeup figure for the low-power regulator with Flash in deep
 * power-down does not describe this bit combination. Wake time and VBUS
 * suspend current are unmeasured. Do not treat either as a board result.
 *
 * VOS and DBP are preserved. PDDS, FPDS and MRLVDS are cleared.
 */
#include <stdint.h>
#define MKR_PWR_CR_LPDS (1u << 0)
#define MKR_PWR_CR_PDDS (1u << 1)
#define MKR_PWR_CR_DBP (1u << 8)
#define MKR_PWR_CR_FPDS (1u << 9)
#define MKR_PWR_CR_LPLVDS (1u << 10)
#define MKR_PWR_CR_MRLVDS (1u << 11)
#define MKR_PWR_CR_VOS (3u << 14)
static inline uint32_t mkr_stop_pwr_cr(uint32_t cr)
{
    return (cr & ~(MKR_PWR_CR_PDDS | MKR_PWR_CR_MRLVDS | MKR_PWR_CR_FPDS)) | MKR_PWR_CR_LPDS |
           MKR_PWR_CR_LPLVDS;
}
#endif
