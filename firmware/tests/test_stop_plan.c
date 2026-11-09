#include "stop_plan.h"
#include <assert.h>
/*
 * RM0383 PWR_CR: LPDS+LPLVDS, PDDS clear, FPDS clear.
 * Flash stays powered. This is not a measured Stop current.
 */
int main(void)
{
    const uint32_t cr = mkr_stop_pwr_cr(MKR_PWR_CR_VOS | MKR_PWR_CR_DBP | MKR_PWR_CR_FPDS | MKR_PWR_CR_PDDS |
                                        MKR_PWR_CR_MRLVDS | MKR_PWR_CR_LPDS);
    assert((cr & MKR_PWR_CR_LPDS) != 0u);
    assert((cr & MKR_PWR_CR_LPLVDS) != 0u);
    assert((cr & MKR_PWR_CR_PDDS) == 0u);
    assert((cr & MKR_PWR_CR_FPDS) == 0u);
    assert((cr & MKR_PWR_CR_MRLVDS) == 0u);
    assert((cr & MKR_PWR_CR_VOS) == MKR_PWR_CR_VOS);
    assert((cr & MKR_PWR_CR_DBP) != 0u);
    assert(mkr_stop_pwr_cr(0u) == (MKR_PWR_CR_LPDS | MKR_PWR_CR_LPLVDS));
    return 0;
}
