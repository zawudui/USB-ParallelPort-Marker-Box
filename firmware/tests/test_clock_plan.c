#include "clock_plan.h"
#include <assert.h>
int main(void)
{
    const uint32_t cr = mkr_cr_enable_hse_crystal(0xFFFFFFFFu);
    assert(MKR_SYSCLK_HZ == 84000000u);
    assert(MKR_USB_HZ == 48000000u);
    assert(MKR_PLLM == 25u && MKR_PLLN == 336u && MKR_PLLP == 4u && MKR_PLLQ == 7u);
    assert(MKR_PLLP_FIELD == 1u);
    assert(MKR_PLLCFGR_VALUE == 0x07415419u);
    assert(MKR_HSE_TIMEOUT_MS >= 100u);
    assert(MKR_FLASH_LATENCY_WS == 2u);
    assert((cr & MKR_RCC_CR_HSEBYP) == 0u);
    assert((cr & MKR_RCC_CR_HSEON) != 0u);
    assert((MKR_SYSCLK_HZ / MKR_APB1_DIV) == 42000000u);
    return 0;
}
