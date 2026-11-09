#include "board_crit.h"
#include <stdint.h>
static uint32_t nest;
static uint32_t saved_primask;
void board_crit_lock(void *ctx)
{
    uint32_t primask;
    (void)ctx;
    __asm volatile("mrs %0, primask" : "=r"(primask));
    __asm volatile("cpsid i\n\tdsb\n\tisb" ::: "memory");
    if (nest == 0u) {
        saved_primask = primask;
    }
    nest++;
}
void board_crit_unlock(void *ctx)
{
    (void)ctx;
    if (nest == 0u) {
        return;
    }
    nest--;
    if (nest == 0u) {
        __asm volatile("msr primask, %0" ::"r"(saved_primask) : "memory");
    }
}
