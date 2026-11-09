#include "board_iwdg.h"
#include "board_clock.h"
#include "board_nvic.h"
#include "clock_plan.h"
#include "iwdg_plan.h"
#include "stm32f411xe.h"
#include "bringup_mode.h"
#ifdef MKR_IWDG_HOST_TEST
void mkr_host_iwdg_key(uint32_t key);
#define IWDG_KEY(value) mkr_host_iwdg_key(value)
#else
#define IWDG_KEY(value) (IWDG->KR = (value))
#endif
#if MKR_BENCH_IWDG
/* stage,wake_count,CSR,BDCR,PWR_CR,RTC_CR,RTC_ISR,WUTR,PR,RLR,SR,
 * fault-injection request (0xA5 suppresses ALL kick paths until reset). */
volatile uint32_t g_iwdg_diag[12];
static void iwdg_diag_stage(uint32_t stage)
{
    g_iwdg_diag[0] = stage;
    g_iwdg_diag[2] = RCC->CSR;
    g_iwdg_diag[3] = RCC->BDCR;
    g_iwdg_diag[4] = PWR->CR;
    g_iwdg_diag[5] = RTC->CR;
    g_iwdg_diag[6] = RTC->ISR;
    g_iwdg_diag[7] = RTC->WUTR;
    g_iwdg_diag[8] = IWDG->PR;
    g_iwdg_diag[9] = IWDG->RLR;
    g_iwdg_diag[10] = IWDG->SR;
}
#define IWDG_STAGE(stage) iwdg_diag_stage(stage)
#else
#define IWDG_STAGE(stage) ((void)0)
#endif
/*
 * LSI feeds both the IWDG and the RTC wakeup, so the kick ratio does not
 * depend on where fLSI sits inside DS10314 Table 40. There is no LSE on
 * this board (PC14/PC15 are unused).
 *
 * Kick is a no-op until the start key has been written. SysTick is already
 * running when this function is called; a 0xAAAA store in the middle of the
 * 0x5555 / PR / RLR sequence would lock those registers again (RM0383).
 */
static volatile uint8_t g_iwdg_armed;
static bool rtc_program_wakeup(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR |= PWR_CR_DBP;
    if (!board_wait_set(&PWR->CR, PWR_CR_DBP, MKR_SYSCLK_HZ / 10u)) {
        return false;
    }
    IWDG_STAGE(2u);
    if (((RCC->BDCR & RCC_BDCR_RTCSEL_Msk) != RCC_BDCR_RTCSEL_LSI) ||
        ((RCC->BDCR & RCC_BDCR_RTCEN) == 0u)) {
        /* RTCSEL can change only by resetting the backup domain. */
        RCC->BDCR |= RCC_BDCR_BDRST;
        RCC->BDCR &= ~RCC_BDCR_BDRST;
        RCC->BDCR |= RCC_BDCR_RTCSEL_LSI | RCC_BDCR_RTCEN;
    }
    RTC->WPR = 0xCAu;
    RTC->WPR = 0x53u;
    RTC->CR &= ~RTC_CR_WUTE;
    IWDG_STAGE(3u);
    if (!board_wait_set(&RTC->ISR, RTC_ISR_WUTWF, MKR_SYSCLK_HZ / 10u)) {
        RTC->WPR = 0xFFu;
        return false;
    }
    RTC->CR = (RTC->CR & ~RTC_CR_WUCKSEL_Msk) | MKR_RTC_WUCKSEL_DIV16;
    RTC->WUTR = MKR_RTC_WUTR;
    RTC->ISR = (~(RTC_ISR_WUTF | RTC_ISR_INIT) & 0x0000FFFFu) | (RTC->ISR & RTC_ISR_INIT);
    EXTI->FTSR &= ~MKR_EXTI_RTC_WAKEUP;
    EXTI->RTSR |= MKR_EXTI_RTC_WAKEUP;
    EXTI->PR = MKR_EXTI_RTC_WAKEUP;
    EXTI->IMR |= MKR_EXTI_RTC_WAKEUP;
    RTC->CR |= RTC_CR_WUTIE | RTC_CR_WUTE;
    RTC->WPR = 0xFFu;
    board_nvic_enable(RTC_WKUP_IRQn, 3u);
    return true;
}
bool board_iwdg_start(void)
{
    IWDG_STAGE(1u);
    RCC->CSR |= RCC_CSR_LSION;
    if (!board_wait_set(&RCC->CSR, RCC_CSR_LSIRDY, MKR_SYSCLK_HZ / 10u)) {
        return false;
    }
    if (!rtc_program_wakeup()) {
        return false;
    }
    IWDG_STAGE(4u);
    /* Match ST HAL_IWDG_Init: start before configuration/update polling. */
    IWDG_KEY(MKR_IWDG_KR_START);
    IWDG_KEY(MKR_IWDG_KR_ACCESS);
    IWDG->PR = MKR_IWDG_PR;
    IWDG->RLR = MKR_IWDG_RLR;
    /* APB1 is 42 MHz here, well above 2 * fLSI (ES0287 2.7.3 / 2.7.4). */
    if (!board_wait_clear(&IWDG->SR, MKR_IWDG_SR_PVU | MKR_IWDG_SR_RVU, MKR_SYSCLK_HZ / 10u)) {
        return false;
    }
    IWDG_KEY(MKR_IWDG_KR_KICK);
    g_iwdg_armed = 1u;
    IWDG_STAGE(5u);
    return true;
}
void board_iwdg_kick(void)
{
#if MKR_BENCH_IWDG
    if (g_iwdg_diag[11] == 0xA5u) { return; }
#endif
    if (g_iwdg_armed == 0u) {
        return;
    }
    IWDG_KEY(MKR_IWDG_KR_KICK);
}
void board_rtc_acknowledge_wakeup(void)
{
    PWR->CR |= PWR_CR_DBP;
    RTC->WPR = 0xCAu;
    RTC->WPR = 0x53u;
    RTC->ISR = (~(RTC_ISR_WUTF | RTC_ISR_INIT) & 0x0000FFFFu) | (RTC->ISR & RTC_ISR_INIT);
    RTC->WPR = 0xFFu;
    EXTI->PR = MKR_EXTI_RTC_WAKEUP;
}
void RTC_WKUP_IRQHandler(void)
{
    /* A posted clear can leave a second IRQ entry pending. Only a real WUTF
     * is a periodic wakeup/feed event, matching ST's RTC interrupt policy. */
    EXTI->PR = MKR_EXTI_RTC_WAKEUP;
    if ((RTC->ISR & RTC_ISR_WUTF) == 0u) {
#ifndef MKR_IWDG_HOST_TEST
        __asm volatile("dsb" ::: "memory");
#endif
        return;
    }
#if MKR_BENCH_IWDG
    ++g_iwdg_diag[1];
#endif
    /* Runs from Stop, where the 1 ms tick is frozen. Kick before clearing
     * the flag so a slow clear cannot eat the window. Clocks stay on HSI;
     * restoring the PLL here would dominate the suspend current. */
    board_iwdg_kick();
    board_rtc_acknowledge_wakeup();
#ifndef MKR_IWDG_HOST_TEST
    __asm volatile("dsb" ::: "memory");
#endif
}
