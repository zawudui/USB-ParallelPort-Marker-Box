#ifndef MKR_BOARD_NVIC_H
#define MKR_BOARD_NVIC_H
#include <stdint.h>
/*
 * STM32F411 implements the top 4 bits of each NVIC priority byte.
 * prio 0 is the most urgent. Writing ISER with a 1 enables that IRQ;
 * writing 0 does not disable it.
 *
 * MKR_USB_HOST_TEST redirects the NVIC writes into RAM so the device
 * controller can be linked on the host. Production builds leave it unset.
 */
#ifdef MKR_USB_HOST_TEST
extern volatile uint8_t mkr_host_nvic_ipr[];
extern volatile uint32_t mkr_host_nvic_iser[];
#define MKR_NVIC_IPR (mkr_host_nvic_ipr)
#define MKR_NVIC_ISER (mkr_host_nvic_iser)
#else
#define MKR_NVIC_IPR ((volatile uint8_t *)0xE000E400u)
#define MKR_NVIC_ISER ((volatile uint32_t *)0xE000E100u)
#endif
static inline void board_nvic_enable(uint32_t irqn, uint32_t prio)
{
    volatile uint8_t *ipr = MKR_NVIC_IPR;
    volatile uint32_t *iser = MKR_NVIC_ISER;
    ipr[irqn] = (uint8_t)(prio << 4);
    iser[irqn >> 5] = 1u << (irqn & 31u);
}
#endif
