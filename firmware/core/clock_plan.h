#ifndef MKR_CLOCK_PLAN_H
#define MKR_CLOCK_PLAN_H
/*
 * STM32F411 clock contract for the r7.2 marker box.
 * HSE is a 25 MHz crystal (not bypass): PLLM=25, PLLN=336, PLLP=4 → 84 MHz;
 * PLLQ=7 → 48 MHz USB. Bit positions match RM0383 RCC_CR / RCC_PLLCFGR.
 */
#include <stdint.h>
#define MKR_HSI_HZ 16000000u
#define MKR_HSE_HZ 25000000u
#define MKR_PLLM 25u
#define MKR_PLLN 336u
#define MKR_PLLP 4u
#define MKR_PLLQ 7u
#define MKR_PLLP_FIELD (((MKR_PLLP) / 2u) - 1u)
#define MKR_SYSCLK_HZ (((MKR_HSE_HZ) / (MKR_PLLM)) * (MKR_PLLN) / (MKR_PLLP))
#define MKR_USB_HZ (((MKR_HSE_HZ) / (MKR_PLLM)) * (MKR_PLLN) / (MKR_PLLQ))
#define MKR_HSE_TIMEOUT_MS 100u
#define MKR_PLL_TIMEOUT_MS 100u
#define MKR_FLASH_LATENCY_WS 2u
#define MKR_APB1_DIV 2u
#define MKR_APB2_DIV 1u
/* RCC_CR */
#define MKR_RCC_CR_HSEON (1u << 16)
#define MKR_RCC_CR_HSERDY (1u << 17)
#define MKR_RCC_CR_HSEBYP (1u << 18)
#define MKR_RCC_CR_CSSON (1u << 19)
#define MKR_RCC_CR_PLLON (1u << 24)
#define MKR_RCC_CR_PLLRDY (1u << 25)
/* RCC_PLLCFGR: PLLM[5:0] PLLN[14:6] PLLP[17:16] PLLSRC[22]=HSE PLLQ[27:24] */
#define MKR_PLLCFGR_VALUE                                                          \
    (((MKR_PLLQ) << 24) | (1u << 22) | ((MKR_PLLP_FIELD) << 16) | ((MKR_PLLN) << 6) | (MKR_PLLM))
#define MKR_RCC_CFGR_SW_PLL 0x2u
#define MKR_RCC_CFGR_SWS_PLL 0x8u
#define MKR_RCC_CFGR_PPRE1_DIV2 0x00001000u
_Static_assert(MKR_HSI_HZ == 16000000u, "reset and Stop-wake SYSCLK is 16 MHz HSI");
_Static_assert(MKR_SYSCLK_HZ == 84000000u, "SYSCLK must be 84 MHz");
_Static_assert(MKR_USB_HZ == 48000000u, "USB clock must be 48 MHz");
_Static_assert(MKR_HSE_TIMEOUT_MS >= 100u, "HSE start timeout");
_Static_assert(MKR_PLLP_FIELD == 1u, "PLLP=/4 encodes as 01");
_Static_assert((MKR_PLLCFGR_VALUE & 0x3Fu) == 25u, "PLLM");
_Static_assert(((MKR_PLLCFGR_VALUE >> 6) & 0x1FFu) == 336u, "PLLN");
_Static_assert(((MKR_PLLCFGR_VALUE >> 16) & 0x3u) == 1u, "PLLP field");
_Static_assert(((MKR_PLLCFGR_VALUE >> 22) & 0x1u) == 1u, "PLL source HSE");
_Static_assert(((MKR_PLLCFGR_VALUE >> 24) & 0xFu) == 7u, "PLLQ");
_Static_assert(MKR_PLLCFGR_VALUE == 0x07415419u, "PLLCFGR fingerprint");
_Static_assert(MKR_FLASH_LATENCY_WS == 2u, "2 wait states at 84 MHz, 2.7-3.6 V");
_Static_assert(MKR_SYSCLK_HZ / MKR_APB1_DIV == 42000000u, "APB1 max 42 MHz");
_Static_assert(MKR_SYSCLK_HZ % 1000u == 0u, "1 ms SysTick divides evenly");
/*
 * DWT CYCCNT counts SYSCLK cycles. 1 ms is MKR_SYSCLK_HZ/1000 cycles.
 * board_delay_ms() uses mkr_cycles_from_ms(). A call written as
 * `25u * MKR_SYSCLK_HZ` is 1000 times too long (25 s at 84 MHz).
 *
 * board_delay_cycles() clamps to MKR_DELAY_CYCLES_CAP (100 ms of CYCCNT)
 * on both the DWT path and the software fallback. HSE/PLL timeouts use
 * board_wait_set/clear, not this delay. The fallback loop is iterations,
 * not CYCCNT ticks; it is not a calibrated millisecond.
 *
 * TIM2 clock: RM0383 sets the timer kernel to 2*PCLK1 when the APB
 * prescaler is not 1. PCLK1 is SYSCLK/2, so TIM2 is clocked at SYSCLK.
 * PSC divides that to 1 µs. This is the reference-manual rule, not a
 * scope measurement of a marker pulse.
 */
#define MKR_CYCLES_PER_MS (MKR_SYSCLK_HZ / 1000u)
#define MKR_DELAY_MS_MAX 100u
#define MKR_DELAY_CYCLES_CAP ((MKR_DELAY_MS_MAX) * (MKR_CYCLES_PER_MS))
#define MKR_CYCLES_FROM_MS_CONST(ms) ((uint32_t)(ms) * (MKR_CYCLES_PER_MS))
#define MKR_USB_DEVICE_MODE_WAIT_MS 25u
#define MKR_TIM2_CLK_HZ (MKR_SYSCLK_HZ)
#define MKR_TIM2_PSC ((MKR_TIM2_CLK_HZ / 1000000u) - 1u)
_Static_assert(MKR_CYCLES_PER_MS == 84000u, "1 ms at 84 MHz");
_Static_assert(MKR_CYCLES_FROM_MS_CONST(MKR_USB_DEVICE_MODE_WAIT_MS) == 2100000u, "25 ms");
_Static_assert(MKR_CYCLES_FROM_MS_CONST(25) * 1000u == 25u * MKR_SYSCLK_HZ, "25*SYSCLK is 25 s");
_Static_assert(MKR_CYCLES_FROM_MS_CONST(25) < MKR_DELAY_CYCLES_CAP, "25 ms is under the cap");
_Static_assert(MKR_DELAY_CYCLES_CAP == 8400000u, "100 ms cap");
_Static_assert(MKR_TIM2_PSC == 83u, "TIM2 1 us tick");
_Static_assert(MKR_APB1_DIV != 1u, "TIM2 x2 clock assumes APB prescaler");
static inline uint32_t mkr_cycles_from_ms(uint32_t ms)
{
    if (ms > MKR_DELAY_MS_MAX) {
        ms = MKR_DELAY_MS_MAX;
    }
    return ms * MKR_CYCLES_PER_MS;
}
static inline uint32_t mkr_delay_cycles_clamped(uint32_t cycles)
{
    return (cycles > MKR_DELAY_CYCLES_CAP) ? MKR_DELAY_CYCLES_CAP : cycles;
}
/* Crystal, not bypass. Call only while HSEON is already 0. */
static inline uint32_t mkr_cr_enable_hse_crystal(uint32_t cr)
{
    cr &= ~MKR_RCC_CR_HSEBYP;
    cr |= MKR_RCC_CR_HSEON;
    return cr;
}
#endif
