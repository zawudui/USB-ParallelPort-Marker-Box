#include "board_clock.h"
#include "board_iwdg.h"
#include "iwdg_plan.h"
#include "clock_plan.h"
#include "hse_pins.h"
#include "stm32f411xe.h"
#define DWT_CTRL ((volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT ((volatile uint32_t *)0xE0001004u)
#define SCB_DEMCR ((volatile uint32_t *)0xE000EDFCu)
static bool dwt_ok;
static void dwt_init(void)
{
    *SCB_DEMCR |= (1u << 24);
    *DWT_CYCCNT = 0u;
    *DWT_CTRL |= 1u;
    for (volatile uint32_t i = 0u; i < 20u; i++) {
    }
    dwt_ok = (*DWT_CYCCNT != 0u);
}
void board_delay_cycles(uint32_t cycles)
{
    /*
     * DWT path: CYCCNT ticks, one tick per SYSCLK cycle, after dwt_init().
     * Both paths clamp to MKR_DELAY_CYCLES_CAP (100 ms at 84 MHz) so a
     * caller that passes milliseconds*SYSCLK cannot spin for ~25 s.
     * 25 ms is 2_100_000 cycles and is not clamped.
     *
     * Fallback (CYCCNT did not move): the loop count is iterations, not
     * CYCCNT ticks. A volatile iteration is several cycles, so this stall
     * is longer than the DWT path for the same request and is still not a
     * calibrated millisecond. The old 2_000_000 iteration cap truncated a
     * correct 25 ms cycle count and did not share a unit with CYCCNT.
     */
    cycles = mkr_delay_cycles_clamped(cycles);
    if (cycles == 0u) {
        return;
    }
    if (!dwt_ok) {
        for (volatile uint32_t i = 0u; i < cycles; i++) {
        }
        return;
    }
    const uint32_t start = *DWT_CYCCNT;
    while ((uint32_t)(*DWT_CYCCNT - start) < cycles) {
    }
}
void board_delay_ms(uint32_t ms)
{
    board_delay_cycles(mkr_cycles_from_ms(ms));
}
/* timeout_cycles is a CYCCNT budget when DWT is running. The fallback
 * counts loop iterations instead, same split as board_delay_cycles.
 * Callers pass generous bounds (HSE/PLL). This is not a measured timeout. */
static bool wait_flag(volatile uint32_t *reg, uint32_t mask, bool want_set, uint32_t cycles)
{
    /* Refresh at each wait entry so several individually short register waits
     * cannot accumulate without a watchdog kick. This is a no-op before IWDG
     * arming. CYCCNT follows HCLK; Stop wake starts on 16 MHz HSI, so iwdg_plan
     * derives the service threshold from that phase rather than 84 MHz. */
    board_iwdg_kick();
    if (!dwt_ok) {
        uint32_t service_iterations = 0u;
        for (uint32_t i = 0u; i < cycles; i++) {
            const bool set = ((*reg & mask) != 0u);
            if (set == want_set) {
                return true;
            }
            /* Conservative fallback only; iteration count is not elapsed time. */
            service_iterations++;
            if (mkr_iwdg_service_due_iterations(service_iterations)) {
                board_iwdg_kick();
                service_iterations = 0u;
            }
        }
        return (((*reg & mask) != 0u) == want_set);
    }
    const uint32_t start = *DWT_CYCCNT;
    uint32_t last_kick = start;
    while ((uint32_t)(*DWT_CYCCNT - start) < cycles) {
        if (((*reg & mask) != 0u) == want_set) {
            return true;
        }
        const uint32_t now = *DWT_CYCCNT;
        if (mkr_iwdg_service_due_cycles(now, last_kick)) {
            board_iwdg_kick();
            last_kick = now;
        }
    }
    return (((*reg & mask) != 0u) == want_set);
}
bool board_wait_set(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_cycles)
{
    return wait_flag(reg, mask, true, timeout_cycles);
}
bool board_wait_clear(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_cycles)
{
    return wait_flag(reg, mask, false, timeout_cycles);
}
void board_hse_pins_prepare(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOHEN;
    (void)RCC->AHB1ENR;
    GPIOH->MODER = mkr_hse_apply_moder(GPIOH->MODER);
    GPIOH->PUPDR = mkr_hse_apply_pupdr(GPIOH->PUPDR);
}
bool board_clock_start(void)
{
    dwt_init();
    /* Program scale 1 while on HSI. On F411 the requested scale becomes
     * active only after PLL is enabled; do not wait for VOSRDY here.
     * ST HAL_PWREx_ControlVoltageScaling follows PLLRDY -> VOSRDY. */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR |= PWR_CR_VOS;
    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    {
        bool latency_ok = false;
        for (uint32_t n = 0u; n < 100000u; n++) {
            if ((FLASH->ACR & FLASH_ACR_LATENCY) == FLASH_ACR_LATENCY_2WS) {
                latency_ok = true;
                break;
            }
        }
        if (!latency_ok) {
            return false;
        }
    }
    /* Analog, no pull, while HSE is still off. Then the crystal takes the pins. */
    board_hse_pins_prepare();
    /* HSE must be off before HSEBYP changes. Reset state is off; clear it anyway. */
    RCC->CR &= ~MKR_RCC_CR_HSEON;
    if (!board_wait_clear(&RCC->CR, MKR_RCC_CR_HSERDY, 1600000u)) {
        return false;
    }
    RCC->CR = mkr_cr_enable_hse_crystal(RCC->CR);
    /* 100 ms at the 16 MHz HSI that is still SYSCLK. */
    if (!board_wait_set(&RCC->CR, MKR_RCC_CR_HSERDY, (uint32_t)MKR_HSE_TIMEOUT_MS * 16000u)) {
        RCC->CR &= ~MKR_RCC_CR_HSEON;
        return false;
    }
    if ((RCC->CR & MKR_RCC_CR_HSEBYP) != 0u) {
        RCC->CR &= ~(MKR_RCC_CR_HSEON | MKR_RCC_CR_HSEBYP);
        return false;
    }
    RCC->CR &= ~MKR_RCC_CR_PLLON;
    if (!board_wait_clear(&RCC->CR, MKR_RCC_CR_PLLRDY, 1600000u)) {
        return false;
    }
    RCC->PLLCFGR = MKR_PLLCFGR_VALUE;
    RCC->CR |= MKR_RCC_CR_PLLON;
    if (!board_wait_set(&RCC->CR, MKR_RCC_CR_PLLRDY, (uint32_t)MKR_PLL_TIMEOUT_MS * 16000u)) {
        RCC->CR &= ~MKR_RCC_CR_PLLON;
        RCC->CR &= ~MKR_RCC_CR_HSEON;
        return false;
    }
    /* PLL is running, but SYSCLK is still HSI until the regulator is ready. */
    if (!board_wait_set(&PWR->CSR, PWR_CSR_VOSRDY, 16000000u)) {
        RCC->CR &= ~MKR_RCC_CR_PLLON;
        RCC->CR &= ~MKR_RCC_CR_HSEON;
        return false;
    }
    RCC->CFGR = (RCC->CFGR & ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2 | RCC_CFGR_SW)) |
                MKR_RCC_CFGR_PPRE1_DIV2 | MKR_RCC_CFGR_SW_PLL;
    if (!board_wait_set(&RCC->CFGR, MKR_RCC_CFGR_SWS_PLL, (uint32_t)MKR_PLL_TIMEOUT_MS * 16000u) ||
        (RCC->CFGR & RCC_CFGR_SWS) != MKR_RCC_CFGR_SWS_PLL) {
        RCC->CFGR &= ~RCC_CFGR_SW;
        RCC->CR &= ~MKR_RCC_CR_PLLON;
        RCC->CR &= ~MKR_RCC_CR_HSEON;
        return false;
    }
    RCC->CR |= MKR_RCC_CR_CSSON;
    return true;
}
bool board_clock_restore_after_stop(void)
{
    const bool pll_up = ((RCC->CFGR & RCC_CFGR_SWS) == MKR_RCC_CFGR_SWS_PLL) &&
                        ((RCC->CR & MKR_RCC_CR_PLLRDY) != 0u) &&
                        ((RCC->CR & MKR_RCC_CR_HSERDY) != 0u) &&
                        ((RCC->CR & MKR_RCC_CR_HSEON) != 0u) &&
                        ((RCC->CR & MKR_RCC_CR_HSEBYP) == 0u) &&
                        (RCC->PLLCFGR == MKR_PLLCFGR_VALUE);
    uint32_t ctrl;
    if (pll_up) {
        RCC->CR |= MKR_RCC_CR_CSSON;
        return true;
    }
    /* Stop cleared HSEON and PLLON. The 1 ms tick must not run on HSI with an
     * 84 MHz reload while the crystal restarts. */
    ctrl = SysTick->CTRL;
    SysTick->CTRL = ctrl & ~SysTick_CTRL_TICKINT_Msk;
    if (!board_clock_start()) {
        return false;
    }
    SysTick->LOAD = (MKR_SYSCLK_HZ / 1000u) - 1u;
    SysTick->VAL = 0u;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    return true;
}
