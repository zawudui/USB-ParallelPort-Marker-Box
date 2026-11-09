#define _DEFAULT_SOURCE 1
#include "board_clock.h"
#include "clock_plan.h"
#include "stm32f411xe.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

static bool model_vos_ready;
static unsigned pll_enable_observations;

/* Run the real board_clock.c against mapped registers. The mock implements
 * the documented F411 dependency: VOSRDY is not raised until PLL is enabled.
 * DWT stays stopped, selecting the existing bounded fallback wait loops. */
void board_iwdg_kick(void)
{
    if ((RCC->CR & MKR_RCC_CR_HSEON) != 0u) { RCC->CR |= MKR_RCC_CR_HSERDY; }
    else { RCC->CR &= ~MKR_RCC_CR_HSERDY; }
    if ((RCC->CR & MKR_RCC_CR_PLLON) != 0u) {
        ++pll_enable_observations;
        RCC->CR |= MKR_RCC_CR_PLLRDY;
        if (model_vos_ready) { PWR->CSR |= PWR_CSR_VOSRDY; }
    } else {
        RCC->CR &= ~MKR_RCC_CR_PLLRDY;
        PWR->CSR &= ~PWR_CSR_VOSRDY;
    }
    if ((RCC->CFGR & RCC_CFGR_SW) == MKR_RCC_CFGR_SW_PLL) {
        RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SWS) | MKR_RCC_CFGR_SWS_PLL;
    }
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

static void reset_model(bool vos_ready)
{
    memset((void *)(uintptr_t)0x40000000u, 0, 0x30000u);
    memset((void *)(uintptr_t)0xe0000000u, 0, 0x10000u);
    RCC->CR = 3u; /* HSI on and ready; HSE and PLL are off. */
    model_vos_ready = vos_ready;
    pll_enable_observations = 0u;
}

int main(void)
{
    if (!map_registers(0x40000000u, 0x30000u) || !map_registers(0xe0000000u, 0x10000u)) {
        fputs("Cannot allocate isolated test register pages\n", stderr);
        return 2;
    }
    reset_model(true);
    if (!board_clock_start() || pll_enable_observations == 0u ||
        (PWR->CSR & PWR_CSR_VOSRDY) == 0u ||
        (RCC->CFGR & RCC_CFGR_SWS) != MKR_RCC_CFGR_SWS_PLL) {
        fputs("FAIL: clock startup waited for VOSRDY before enabling PLL\n", stderr);
        return 1;
    }
    reset_model(false);
    if (board_clock_start() || pll_enable_observations == 0u ||
        (RCC->CR & (MKR_RCC_CR_PLLON | MKR_RCC_CR_HSEON)) != 0u ||
        (RCC->CFGR & RCC_CFGR_SW) != 0u) {
        fputs("FAIL: missing VOSRDY did not leave the system on HSI with HSE/PLL off\n", stderr);
        return 1;
    }
    puts("PASS: PLL precedes VOSRDY; VOS timeout fails closed on HSI");
    return 0;
}
