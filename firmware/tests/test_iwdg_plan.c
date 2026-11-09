#include "clock_plan.h"
#include "iwdg_plan.h"
#include <assert.h>
int main(void)
{
    assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MAX) >= 100u);
    assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MAX) > MKR_HSE_TIMEOUT_MS);
    assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MAX) >= MKR_HSE_TIMEOUT_MS + 40u);
    assert(MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MIN) > MKR_IWDG_TIMEOUT_MS_AT(MKR_LSI_HZ_MAX));
    assert(MKR_RTC_KICK_LSI_COUNTS * 4u == MKR_IWDG_LSI_COUNTS);
    assert(MKR_RTC_KICK_LSI_COUNTS * 3u < MKR_IWDG_LSI_COUNTS);
    assert(MKR_RTC_KICK_LSI_COUNTS * 4u == MKR_IWDG_LSI_COUNTS);
    assert(mkr_iwdg_stop_allowed(0u));
    assert(!mkr_iwdg_stop_allowed(MKR_IWDG_SR_PVU));
    assert(!mkr_iwdg_stop_allowed(MKR_IWDG_SR_RVU));
    assert(!mkr_iwdg_stop_allowed(MKR_IWDG_SR_PVU | MKR_IWDG_SR_RVU));
    assert(MKR_IWDG_KR_KICK == 0xAAAAu);
    assert(MKR_IWDG_KR_START == 0xCCCCu);
    return 0;
}
