#ifndef MKR_BRINGUP_H
#define MKR_BRINGUP_H
#include <stdbool.h>
/*
 * Order is part of the safety contract:
 *   gpio_safe, runtime_init, clock, usb, then the 1 ms tick.
 * Clock or USB start failure latches safe-off and halt. halt may not return.
 */
typedef struct {
    void (*gpio_safe)(void);
    void (*runtime_init)(void);
    bool (*clock_start)(void);
    void (*latch_safe_fault)(void);
    bool (*usb_start)(void);
    void (*timebase_start)(void);
    void (*halt)(void);
} mkr_bringup_ops_t;
void mkr_bringup(const mkr_bringup_ops_t *ops);
#endif
