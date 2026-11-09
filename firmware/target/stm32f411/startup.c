#include "board_gpio.h"
#include "stm32f411xe.h"
#include <stddef.h>
#include <stdint.h>
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
void Reset_Handler(void);
void NMI_Handler(void);
void HardFault_Handler(void);
void Default_Handler(void);
void SysTick_Handler(void);
void OTG_FS_IRQHandler(void);
void RTC_WKUP_IRQHandler(void);
void OTG_FS_WKUP_IRQHandler(void);
void TIM2_IRQHandler(void);
bool mkr_wakeup_vectors_installed(void);
void mkr_target_nmi_clock_fault(void);
typedef void (*irq_fn)(void);
#define R ((irq_fn)0)
__attribute__((used, section(".isr_vector"), aligned(512)))
irq_fn const g_pfnVectors[102] = {
    (irq_fn)(uintptr_t)&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    R, R, R, R,
    Default_Handler,
    Default_Handler,
    R,
    Default_Handler,
    SysTick_Handler,
    /* IRQ 0 WWDG, 1 PVD, 2 TAMP, 3 RTC_WKUP, 4 FLASH */
    Default_Handler, Default_Handler, Default_Handler, RTC_WKUP_IRQHandler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    /* 19 .. 22 reserved */
    R, R, R, R,
    /* 23 .. 27, then 28 TIM2, 29 TIM3, 30 TIM4, 31 I2C1_EV, 32 I2C1_ER */
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    TIM2_IRQHandler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    Default_Handler,
    /* 39 reserved */
    R,
    /* 40 EXTI16 / 41 EXTI17 / 42 OTG_FS_WKUP */
    Default_Handler, Default_Handler, OTG_FS_WKUP_IRQHandler,
    /* 43 .. 46 reserved */
    R, R, R, R,
    /* 47 */
    Default_Handler,
    /* 48 reserved */
    R,
    /* 49 .. 51 */
    Default_Handler, Default_Handler, Default_Handler,
    /* 52 .. 55 reserved */
    R, R, R, R,
    /* 56 .. 60 */
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    /* 61 .. 66 reserved */
    R, R, R, R, R, R,
    /* 67 OTG_FS */
    OTG_FS_IRQHandler,
    /* 68 .. 73 */
    Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler, Default_Handler,
    /* 74 .. 80 reserved */
    R, R, R, R, R, R, R,
    /* 81 FPU */
    Default_Handler,
    /* 82 .. 83 reserved */
    R, R,
    /* 84 .. 85 SPI4, SPI5 */
    Default_Handler, Default_Handler};
_Static_assert(sizeof g_pfnVectors / sizeof g_pfnVectors[0] == 102u, "vector table");
bool mkr_wakeup_vectors_installed(void)
{
    return g_pfnVectors[15] == SysTick_Handler &&
           g_pfnVectors[16 + 3] == RTC_WKUP_IRQHandler &&
           g_pfnVectors[16 + 28] == TIM2_IRQHandler &&
           g_pfnVectors[16 + 42] == OTG_FS_WKUP_IRQHandler &&
           g_pfnVectors[16 + 67] == OTG_FS_IRQHandler;
}
void Reset_Handler(void)
{
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0u;
    }
    *((volatile uint32_t *)0xE000ED08u) = 0x08000000u;
    extern int main(void);
    (void)main();
    for (;;) {
    }
}
void NMI_Handler(void)
{
    if ((RCC->CIR & RCC_CIR_CSSF) != 0u) {
        RCC->CIR |= RCC_CIR_CSSC;
    }
    mkr_target_nmi_clock_fault();
    for (;;) {
        __asm volatile("wfi");
    }
}
void HardFault_Handler(void)
{
    board_gpio_force_safe_off();
    mkr_target_nmi_clock_fault();
    for (;;) {
    }
}
void Default_Handler(void)
{
    board_gpio_force_safe_off();
    mkr_target_nmi_clock_fault();
    for (;;) {
    }
}
void *memcpy(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    for (size_t i = 0u; i < n; i++) {
        d[i] = s[i];
    }
    return dst;
}
void *memset(void *dst, int c, size_t n)
{
    uint8_t *d = dst;
    for (size_t i = 0u; i < n; i++) {
        d[i] = (uint8_t)c;
    }
    return dst;
}
void *memmove(void *dst, const void *src, size_t n)
{
    uint8_t *d = dst;
    const uint8_t *s = src;
    if (d == s || n == 0u) {
        return dst;
    }
    if (d < s) {
        return memcpy(dst, src, n);
    }
    for (size_t i = n; i > 0u; i--) {
        d[i - 1u] = s[i - 1u];
    }
    return dst;
}
void __aeabi_memcpy(void *dst, const void *src, size_t n) { (void)memcpy(dst, src, n); }
void __aeabi_memcpy4(void *dst, const void *src, size_t n) { (void)memcpy(dst, src, n); }
void __aeabi_memcpy8(void *dst, const void *src, size_t n) { (void)memcpy(dst, src, n); }
void __aeabi_memmove(void *dst, const void *src, size_t n) { (void)memmove(dst, src, n); }
void __aeabi_memset(void *dst, size_t n, int c) { (void)memset(dst, c, n); }
void __aeabi_memclr(void *dst, size_t n) { (void)memset(dst, 0, n); }
void __aeabi_memclr4(void *dst, size_t n) { (void)memset(dst, 0, n); }
void __aeabi_memclr8(void *dst, size_t n) { (void)memset(dst, 0, n); }
