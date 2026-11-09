#include "board_time.h"
#include "board_nvic.h"
#include "clock_plan.h"
#include "time_us.h"
#include "stm32f411xe.h"
#include <stdbool.h>
static void (*g_cb)(void *);
static void *g_ctx;
static bool g_time_on;
uint32_t mkr_hw_time_us(void)
{
    if (!g_time_on) {
        return 0u;
    }
    return TIM2->CNT;
}
void mkr_hw_disarm_deadline(void)
{
    if (!g_time_on) {
        return;
    }
    TIM2->DIER &= ~TIM_DIER_CC1IE;
    TIM2->SR &= ~TIM_SR_CC1IF;
}
void mkr_hw_arm_deadline(uint32_t deadline_us)
{
    if (!g_time_on) {
        return;
    }
    TIM2->CCR1 = deadline_us;
    TIM2->SR &= ~TIM_SR_CC1IF;
    TIM2->DIER |= TIM_DIER_CC1IE;
    /* A compare that is already due does not wait for the next wrap. */
    if ((int32_t)(TIM2->CNT - deadline_us) >= 0) {
        TIM2->EGR = TIM_EGR_CC1G;
    }
}
void board_time_start(void (*on_deadline)(void *ctx), void *ctx)
{
    g_cb = on_deadline;
    g_ctx = ctx;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;
    TIM2->CR1 = 0u;
    TIM2->PSC = MKR_TIM2_PSC;
    TIM2->ARR = 0xFFFFFFFFu;
    TIM2->CNT = 0u;
    TIM2->DIER = 0u;
    TIM2->CCER = 0u; /* do not drive TIM2_CH1; PA15 stays the GPIO power enable */
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0u;
    TIM2->CR1 = TIM_CR1_CEN;
    g_time_on = true;
    board_nvic_enable(TIM2_IRQn, 1u);
}
void TIM2_IRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_CC1IF) == 0u) {
        return;
    }
    TIM2->SR &= ~TIM_SR_CC1IF;
    TIM2->DIER &= ~TIM_DIER_CC1IE;
    if (g_cb != 0) {
        g_cb(g_ctx);
    }
}
