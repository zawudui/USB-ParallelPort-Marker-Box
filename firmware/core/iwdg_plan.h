#ifndef MKR_IWDG_PLAN_H
#define MKR_IWDG_PLAN_H
/*
 * Independent-watchdog window for the review image.
 *
 * DS10314 Rev 8 Table 40: fLSI is 17 / 32 / 47 kHz (min / typ / max),
 * characterized, not production-tested. The timeout below is that arithmetic.
 * It has not been measured on this board.
 *
 * Counts are in LSI cycles so the RTC kick, which uses the same LSI, stays
 * at 1/4 of the IWDG window at every frequency in the table. Two consecutive
 * missed RTC wakeups still leave the third scheduled wake at 3/4 of the window.
 * Three missed wakeups reach the nominal timeout boundary; do not rely on it.
 *
 * RM0383 IWDG: prescaler /64 is PR[2:0] = 100. Timeout = (RLR+1) * prescaler / fLSI.
 * Keys: 0x5555 unlock, 0xAAAA kick, 0xCCCC start. Start is one-way until reset.
 *
 * ES0287 2.7.1 / 2.7.2: do not enter Stop while PVU or RVU is set.
 */
#include "clock_plan.h"
#include <stdbool.h>
#include <stdint.h>
#define MKR_LSI_HZ_MIN 17000u
#define MKR_LSI_HZ_TYP 32000u
#define MKR_LSI_HZ_MAX 47000u
#define MKR_IWDG_PR 4u
#define MKR_IWDG_PRESCALER 64u
#define MKR_IWDG_RLR 110u
#define MKR_IWDG_LSI_COUNTS ((MKR_IWDG_RLR + 1u) * MKR_IWDG_PRESCALER)
#define MKR_IWDG_TIMEOUT_MS_AT(hz) ((MKR_IWDG_LSI_COUNTS * 1000u) / (hz))
#define MKR_RTC_KICK_DIVISOR 4u
#define MKR_RTC_KICK_LSI_COUNTS (MKR_IWDG_LSI_COUNTS / MKR_RTC_KICK_DIVISOR)
#define MKR_RTC_WUT_DIV 16u
#define MKR_RTC_WUCKSEL_DIV16 0u
#define MKR_RTC_WUTR ((MKR_RTC_KICK_LSI_COUNTS / MKR_RTC_WUT_DIV) - 1u)
#define MKR_IWDG_KR_ACCESS 0x5555u
#define MKR_IWDG_KR_KICK 0xAAAAu
#define MKR_IWDG_KR_START 0xCCCCu
#define MKR_IWDG_SR_PVU (1u << 0)
#define MKR_IWDG_SR_RVU (1u << 1)
/* DWT CYCCNT follows HCLK. Stop wake and clock restoration start on HSI,
 * so this is 25 ms at the slowest clock phase; after the switch to 84 MHz
 * the same cycle threshold is shorter. The fallback is deliberately frequent
 * but remains an iteration budget, not a calibrated time interval. */
#define MKR_IWDG_SERVICE_INTERVAL_US 25000u
#define MKR_IWDG_SERVICE_INTERVAL_CYCLES ((MKR_HSI_HZ / 1000000u) * MKR_IWDG_SERVICE_INTERVAL_US)
#define MKR_IWDG_SERVICE_FALLBACK_ITERATIONS 10000u
static inline bool mkr_iwdg_service_due_cycles(uint32_t now, uint32_t last)
{
    return (uint32_t)(now - last) >= MKR_IWDG_SERVICE_INTERVAL_CYCLES;
}
static inline bool mkr_iwdg_service_due_iterations(uint32_t iterations)
{
    return iterations >= MKR_IWDG_SERVICE_FALLBACK_ITERATIONS;
}
static inline bool mkr_iwdg_stop_allowed(uint32_t sr)
{
    return (sr & (MKR_IWDG_SR_PVU | MKR_IWDG_SR_RVU)) == 0u;
}
_Static_assert((4u << MKR_IWDG_PR) == MKR_IWDG_PRESCALER, "PR=4 is divider 64");
_Static_assert(MKR_IWDG_RLR <= 0x0FFFu, "RLR is 12 bits");
_Static_assert(MKR_IWDG_LSI_COUNTS == 7104u, "IWDG LSI counts");
_Static_assert((uint64_t)MKR_IWDG_SERVICE_INTERVAL_CYCLES * 5u <
                   ((uint64_t)MKR_IWDG_LSI_COUNTS * MKR_HSI_HZ) / MKR_LSI_HZ_MAX,
               "five HSI-phase service intervals fit inside the fastest-LSI IWDG window");
_Static_assert(MKR_RTC_KICK_LSI_COUNTS * MKR_RTC_KICK_DIVISOR == MKR_IWDG_LSI_COUNTS, "kick is 1/4 window");
_Static_assert(MKR_RTC_KICK_LSI_COUNTS * 3u < MKR_IWDG_LSI_COUNTS, "two missed RTC wakeups still leave margin");
_Static_assert(MKR_RTC_KICK_LSI_COUNTS % MKR_RTC_WUT_DIV == 0u, "RTC/16 divides the kick");
_Static_assert(MKR_RTC_WUTR == 110u, "WUTR");
_Static_assert(MKR_RTC_WUTR <= 0xFFFFu, "WUTR is 16 bits");
_Static_assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MAX) >= 100u, "window at fastest LSI");
_Static_assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MAX) >= 140u, "margin over the 100 ms HSE wait");
_Static_assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_TYP) == 222u, "typical floored ms");
_Static_assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MIN) == 417u, "slowest floored ms");
_Static_assert(MKR_IWDG_KR_ACCESS != MKR_IWDG_KR_KICK, "kick must not unlock");
_Static_assert(MKR_IWDG_KR_KICK != MKR_IWDG_KR_START, "kick must not start");
_Static_assert(MKR_IWDG_KR_ACCESS != MKR_IWDG_KR_START, "keys differ");
#endif
