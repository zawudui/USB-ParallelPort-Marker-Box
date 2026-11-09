#include "board_stop.h"
#include "board_iwdg.h"
#include "board_clock.h"
#include "board_gpio.h"
#include "iwdg_plan.h"
#include "stop_plan.h"
#include "stm32f411xe.h"
#include "usb_dcd.h"
#include "bringup_mode.h"
#if MKR_BENCH_STOP
/* entries,returns,USB wake IRQs,clock restores,last wake CR/CFGR/SCR/RTC_ISR */
volatile uint32_t g_stop_diag[8];
#endif
/*
 * STOP LPLV keeps SRAM, so the USB device state survives. After Stop, RM0383
 * selects HSI and clears HSEON and PLLON. USB resume is EXTI line 18; the
 * OTG wakeup IRQ restores the crystal PLL before the OTG IRQ reads GINTSTS.
 *
 * Enter with PRIMASK set. WFI still wakes, but the ISR stays pending until
 * SLEEPDEEP is clear, so the handler cannot go back to sleep as Stop.
 * ES0287: if PVU or RVU is set, skip Stop for this pass. The main loop kicks
 * and tries again. Those flags are cleared at arming time and the prescaler
 * is not rewritten afterwards.
 *
 * FPDS is cleared: Flash is not in deep power-down. DS10314's wakeup
 * number for Flash power-down does not apply. Nothing here is a measured
 * suspend current or a measured wake time.
 */
_Static_assert(MKR_PWR_CR_LPDS == PWR_CR_LPDS, "LPDS bit");
_Static_assert(MKR_PWR_CR_PDDS == PWR_CR_PDDS, "PDDS bit");
_Static_assert(MKR_PWR_CR_FPDS == PWR_CR_FPDS, "FPDS bit");
_Static_assert(MKR_PWR_CR_LPLVDS == PWR_CR_LPLVDS, "LPLVDS bit");
_Static_assert(MKR_PWR_CR_MRLVDS == PWR_CR_MRLVDS, "MRLVDS bit");
_Static_assert(MKR_PWR_CR_DBP == PWR_CR_DBP, "DBP bit");
_Static_assert((MKR_PWR_CR_VOS) == PWR_CR_VOS, "VOS field");
#define SCB_SCR (*(volatile uint32_t *)0xE000ED10u)
#define SCB_SCR_SLEEPONEXIT (1u << 1)
#define SCB_SCR_SLEEPDEEP (1u << 2)
extern void mkr_target_nmi_clock_fault(void);
void board_stop_clear_sleepdeep(void)
{
    SCB_SCR &= ~(SCB_SCR_SLEEPDEEP | SCB_SCR_SLEEPONEXIT);
}
void board_stop_enter(void)
{
    uint32_t primask;
    if (!mkr_iwdg_stop_allowed(IWDG->SR)) {
        __asm volatile("wfi" ::: "memory");
        return;
    }
    __asm volatile("mrs %0, primask" : "=r"(primask));
    __asm volatile("cpsid i\n\tdsb\n\tisb" ::: "memory");
    if (!mkr_usb_dcd_stop_requested()) {
        __asm volatile("msr primask, %0" ::"r"(primask) : "memory");
        return;
    }
    PWR->CR = mkr_stop_pwr_cr(PWR->CR);
    board_iwdg_kick();
    board_rtc_acknowledge_wakeup();
    SCB_SCR = (SCB_SCR & ~SCB_SCR_SLEEPONEXIT) | SCB_SCR_SLEEPDEEP;
#if MKR_BENCH_STOP
    ++g_stop_diag[0];
#endif
    __asm volatile("dsb\n\tisb\n\twfi\n\tdsb\n\tisb" ::: "memory");
    board_stop_clear_sleepdeep();
#if MKR_BENCH_STOP
    ++g_stop_diag[1];
    g_stop_diag[4] = RCC->CR;
    g_stop_diag[5] = RCC->CFGR;
    g_stop_diag[6] = SCB_SCR;
    g_stop_diag[7] = RTC->ISR;
#endif
    __asm volatile("msr primask, %0" ::"r"(primask) : "memory");
}
void OTG_FS_WKUP_IRQHandler(void)
{
#if MKR_BENCH_STOP
    ++g_stop_diag[2];
#endif
    board_iwdg_kick();
    board_stop_clear_sleepdeep();
    EXTI->PR = MKR_EXTI_USB_WAKEUP;
    if (!board_clock_restore_after_stop()) {
        mkr_target_nmi_clock_fault();
        for (;;) {
            __asm volatile("wfi");
        }
    }
#if MKR_BENCH_STOP
    ++g_stop_diag[3];
#endif
    /* ISO power stays with the power sequence. A still-idle bus raises
     * suspend again; a bus reset is left pending for the OTG IRQ. */
    mkr_usb_dcd_on_exti_wakeup();
}
