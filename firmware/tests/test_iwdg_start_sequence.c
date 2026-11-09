#define _DEFAULT_SOURCE 1
#include "board_iwdg.h"
#include "board_clock.h"
#include "iwdg_plan.h"
#include "stm32f411xe.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
extern volatile uint32_t g_iwdg_diag[12];
static bool started, unlocked;
static unsigned kicks;
static unsigned failure;
void mkr_host_iwdg_key(uint32_t key)
{
    if (key == MKR_IWDG_KR_START) { started = true; }
    else if (key == MKR_IWDG_KR_ACCESS) {
        if (!started) { fputs("FAIL: IWDG configured before start\n", stderr); _Exit(1); }
        unlocked = true;
    } else if (key == MKR_IWDG_KR_KICK) {
        assert(started);
        ++kicks;
        unlocked = false;
    }
    IWDG->KR = key;
}
bool board_wait_set(volatile uint32_t *reg, uint32_t mask, uint32_t timeout)
{
    (void)timeout;
    if (reg == &RCC->CSR && failure == 1u) { return false; }
    if (reg == &RTC->ISR && failure == 2u) { return false; }
    *reg |= mask;
    return true;
}
bool board_wait_clear(volatile uint32_t *reg, uint32_t mask, uint32_t timeout)
{
    (void)timeout;
    assert(reg == &IWDG->SR && started && unlocked);
    assert(IWDG->PR == MKR_IWDG_PR && IWDG->RLR == MKR_IWDG_RLR);
    if (failure == 3u) { return false; }
    *reg &= ~mask;
    return true;
}
static bool map_registers(uintptr_t address, size_t size)
{
#ifdef _WIN32
    return VirtualAlloc((void *)address, size, MEM_RESERVE | MEM_COMMIT,
                        PAGE_READWRITE) == (void *)address;
#else
    return mmap((void *)address, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0) == (void *)address;
#endif
}
static void reset_model(unsigned fail)
{
    memset((void *)(uintptr_t)0x40000000u, 0, 0x30000u);
    started = unlocked = false;
    kicks = 0u;
    failure = fail;
}
int main(void)
{
    if (!map_registers(0x40000000u, 0x30000u) || !map_registers(0xe0000000u, 0x10000u)) {
        fputs("Cannot map test registers\n", stderr); return 2;
    }
    /* Failure paths must never publish an armed watchdog to asynchronous kicks. */
    for (unsigned fail = 1u; fail <= 3u; ++fail) {
        reset_model(fail);
        assert(!board_iwdg_start());
        board_iwdg_kick();
        assert(kicks == 0u);
        if (fail < 3u) { assert(!started); }
    }
    reset_model(0u);
    assert(board_iwdg_start());
    assert(started && kicks == 1u);
    assert(RTC->WUTR == MKR_RTC_WUTR);
    assert((RTC->CR & (RTC_CR_WUTE | RTC_CR_WUTIE)) == (RTC_CR_WUTE | RTC_CR_WUTIE));
    assert(g_iwdg_diag[0] == 5u);
    board_iwdg_kick();
    assert(kicks == 2u);
    g_iwdg_diag[11] = 0xA5u;
    board_iwdg_kick();
    assert(kicks == 2u); /* fault injection suppresses the common feeding path */
    g_iwdg_diag[11] = 0u;
    board_iwdg_kick();
    assert(kicks == 3u);
    RTC->ISR |= RTC_ISR_WUTF;
    RTC_WKUP_IRQHandler();
    assert(kicks == 4u && g_iwdg_diag[1] == 1u);
    RTC_WKUP_IRQHandler();
    assert(kicks == 4u && g_iwdg_diag[1] == 1u);
    puts("PASS: IWDG start/config/update/reload ordering, RTC setup and bounded failures");
    return 0;
}
