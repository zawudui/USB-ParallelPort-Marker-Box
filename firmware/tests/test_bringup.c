#include "bringup.h"
#include <assert.h>
#include <string.h>
static char logbuf[64];
static unsigned n;
static bool clock_ok_flag;
static bool usb_ok_flag;
static void add(char c)
{
    assert(n < sizeof logbuf);
    logbuf[n++] = c;
}
static void gpio(void) { add('G'); }
static void runtime(void) { add('R'); }
static bool clock(void)
{
    add('C');
    return clock_ok_flag;
}
static void fault(void) { add('F'); }
static bool usb(void)
{
    add('U');
    return usb_ok_flag;
}
static void timebase(void) { add('T'); }
static void halt(void) { add('H'); }
int main(void)
{
    const mkr_bringup_ops_t ops = {gpio, runtime, clock, fault, usb, timebase, halt};
    clock_ok_flag = true;
    usb_ok_flag = true;
    n = 0u;
    mkr_bringup(&ops);
    assert(n == 5u && memcmp(logbuf, "GRCUT", 5u) == 0);
    clock_ok_flag = false;
    n = 0u;
    mkr_bringup(&ops);
    assert(n == 5u && memcmp(logbuf, "GRCFH", 5u) == 0);
    assert(memchr(logbuf, 'U', n) == 0 && memchr(logbuf, 'T', n) == 0);
    clock_ok_flag = true;
    usb_ok_flag = false;
    n = 0u;
    mkr_bringup(&ops);
    assert(memcmp(logbuf, "GRCUFH", 6u) == 0);
    assert(memchr(logbuf, 'T', n) == 0);
    return 0;
}
