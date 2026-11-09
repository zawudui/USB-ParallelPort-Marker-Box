#include "bringup.h"
void mkr_bringup(const mkr_bringup_ops_t *ops)
{
    ops->gpio_safe();
    ops->runtime_init();
    if (!ops->clock_start()) {
        ops->latch_safe_fault();
        ops->halt();
        return;
    }
    if (!ops->usb_start()) {
        ops->latch_safe_fault();
        ops->halt();
        return;
    }
    ops->timebase_start();
}
