#include "clock_plan.h"
#include "iwdg_plan.h"
#include <assert.h>
#include <stdint.h>
int main(void)
{
    const uint32_t interval = MKR_IWDG_SERVICE_INTERVAL_CYCLES;
    const uint64_t fastest_iwdg_hsi_cycles =
        ((uint64_t)MKR_IWDG_LSI_COUNTS * (uint64_t)MKR_HSI_HZ) / MKR_LSI_HZ_MAX;
    assert(interval == 400000u);
    assert(!mkr_iwdg_service_due_cycles(interval - 1u, 0u));
    assert(mkr_iwdg_service_due_cycles(interval, 0u));
    /* Unsigned CYCCNT subtraction remains correct across the 32-bit wrap. */
    assert(!mkr_iwdg_service_due_cycles(0x0006197Fu, 0xFFFFFF00u));
    assert(mkr_iwdg_service_due_cycles(0x00061980u, 0xFFFFFF00u));
    assert(!mkr_iwdg_service_due_iterations(MKR_IWDG_SERVICE_FALLBACK_ITERATIONS - 1u));
    assert(mkr_iwdg_service_due_iterations(MKR_IWDG_SERVICE_FALLBACK_ITERATIONS));
    assert(mkr_iwdg_service_due_iterations(MKR_IWDG_SERVICE_FALLBACK_ITERATIONS + 1u));
    /* During Stop wake, CYCCNT follows the 16 MHz HSI until PLL switch.
     * Five 25 ms service periods fit inside the fastest-LSI nominal window
     * (~151 ms). Cast before multiplication to prevent uint32 overflow. */
    assert((uint64_t)interval * 5u < fastest_iwdg_hsi_cycles);
    assert(fastest_iwdg_hsi_cycles > 150u * (uint64_t)MKR_HSI_HZ / 1000u);
    /* After switching to 84 MHz, the same CYCCNT threshold is shorter. */
    assert((uint64_t)interval * 1000u / MKR_SYSCLK_HZ < 5u);
    /* RTC wakeups are spaced at one quarter of the nominal IWDG window:
     * two missed wakeups leave the third scheduled wake at 3/4; three
     * missed wakeups reach the nominal timeout boundary. */
    assert(MKR_RTC_KICK_LSI_COUNTS * 4u == MKR_IWDG_LSI_COUNTS);
    assert(MKR_RTC_KICK_LSI_COUNTS * 3u < MKR_IWDG_LSI_COUNTS);
    return 0;
}
