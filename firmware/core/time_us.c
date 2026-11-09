#include "time_us.h"
static uint32_t g_soft_us;
__attribute__((weak)) uint32_t mkr_hw_time_us(void)
{
    return g_soft_us;
}
__attribute__((weak)) void mkr_hw_arm_deadline(uint32_t deadline_us)
{
    (void)deadline_us;
}
__attribute__((weak)) void mkr_hw_disarm_deadline(void) {}
void mkr_time_soft_set_us(uint32_t now_us)
{
    g_soft_us = now_us;
}
void mkr_time_soft_advance_us(uint32_t delta_us)
{
    g_soft_us += delta_us;
}
