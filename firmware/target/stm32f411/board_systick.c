#include "board_systick.h"
#include "board_iwdg.h"
#include "clock_plan.h"
#include "stm32f411xe.h"
static void (*g_tick)(void *);
static void *g_ctx;
void board_systick_start(void (*tick)(void *ctx), void *ctx)
{
    volatile uint8_t *systick_prio = (volatile uint8_t *)0xE000ED23u;
    g_tick = tick;
    g_ctx = ctx;
    SysTick->CTRL = 0u;
    SysTick->LOAD = (MKR_SYSCLK_HZ / 1000u) - 1u;
    SysTick->VAL = 0u;
    *systick_prio = (uint8_t)(2u << 4); /* lower urgency than USB (priority 1) */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}
void SysTick_Handler(void)
{
    /* Before the application tick, and a no-op until board_iwdg_start(). */
    board_iwdg_kick();
    if (g_tick != 0) {
        g_tick(g_ctx);
    }
}
