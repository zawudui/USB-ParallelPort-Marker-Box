#include "clock_plan.h"
#include <assert.h>
/*
 * Locks the millisecond-to-CYCCNT conversion used by board_delay_ms().
 * Passing 25 * MKR_SYSCLK_HZ is 25 seconds at 84 MHz, not 25 ms.
 * This host test does not run on the chip and does not measure DWT.
 */
int main(void)
{
    assert(MKR_SYSCLK_HZ == 84000000u);
    assert(MKR_CYCLES_PER_MS == 84000u);
    assert(MKR_USB_DEVICE_MODE_WAIT_MS == 25u);
    assert(mkr_cycles_from_ms(25u) == 2100000u);
    assert(mkr_cycles_from_ms(25u) == 25u * (MKR_SYSCLK_HZ / 1000u));
    assert(mkr_cycles_from_ms(MKR_USB_DEVICE_MODE_WAIT_MS) * 1000u == 25u * MKR_SYSCLK_HZ);
    assert(mkr_cycles_from_ms(25u) != 25u * MKR_SYSCLK_HZ);
    assert(mkr_delay_cycles_clamped(mkr_cycles_from_ms(25u)) == mkr_cycles_from_ms(25u));
    assert(mkr_delay_cycles_clamped(64u) == 64u);
    assert(mkr_delay_cycles_clamped(0u) == 0u);
    /* The old call saturates at the 100 ms cap instead of spinning ~25 s. */
    assert(mkr_delay_cycles_clamped(25u * MKR_SYSCLK_HZ) == MKR_DELAY_CYCLES_CAP);
    assert(MKR_DELAY_CYCLES_CAP / MKR_CYCLES_PER_MS == 100u);
    assert(mkr_cycles_from_ms(101u) == mkr_cycles_from_ms(MKR_DELAY_MS_MAX));
    assert(mkr_cycles_from_ms(100000u) == 100u * MKR_CYCLES_PER_MS);
    assert(MKR_TIM2_PSC == 83u);
    return 0;
}
