#include "clock_plan.h"
#include "hse_pins.h"
#include <assert.h>
int main(void)
{
    const uint32_t moder = mkr_hse_apply_moder(0xFFFFFFFFu);
    const uint32_t pupdr = mkr_hse_apply_pupdr(0xFFFFFFFFu);
    const uint32_t cr = mkr_cr_enable_hse_crystal(0xFFFFFFFFu);
    assert((moder & MKR_HSE_PIN_MODER_MASK) == 0x0000000Fu);
    assert((moder & ~MKR_HSE_PIN_MODER_MASK) == 0xFFFFFFF0u);
    assert((pupdr & MKR_HSE_PIN_MODER_MASK) == 0u);
    assert((pupdr & ~MKR_HSE_PIN_MODER_MASK) == 0xFFFFFFF0u);
    assert(mkr_hse_apply_moder(0u) == 0x0000000Fu);
    assert(mkr_hse_apply_pupdr(0u) == 0u);
    /* Pin programming does not select bypass. Crystal enable still clears it. */
    assert((cr & MKR_RCC_CR_HSEBYP) == 0u);
    assert((cr & MKR_RCC_CR_HSEON) != 0u);
    assert(MKR_PLLM == 25u && MKR_PLLN == 336u && MKR_PLLP == 4u && MKR_PLLQ == 7u);
    assert(MKR_SYSCLK_HZ == 84000000u && MKR_USB_HZ == 48000000u);
    assert(MKR_HSE_TIMEOUT_MS == 100u);
    return 0;
}
