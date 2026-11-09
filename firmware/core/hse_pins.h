#ifndef MKR_HSE_PINS_H
#define MKR_HSE_PINS_H
/*
 * PH0 (OSC_IN) and PH1 (OSC_OUT) before HSEON.
 *
 * RM0383 §8.3.12: analog mode disables the output buffer, the Schmitt trigger
 * and the weak pulls. §8.3.14: those pins are GPIO only while HSE is off;
 * setting HSEON gives them to the oscillator and the oscillator wins over GPIO.
 * Program analog and no-pull first, then enable the crystal (HSEBYP stays 0).
 * This does not change the 25 MHz PLL plan.
 */
#include <stdint.h>
#define MKR_HSE_PIN_PH0 0u
#define MKR_HSE_PIN_PH1 1u
#define MKR_GPIO_MODE_ANALOG 3u
#define MKR_HSE_PIN_MODER_MASK 0x0000000Fu
static inline uint32_t mkr_hse_apply_moder(uint32_t moder)
{
    const uint32_t analog = (MKR_GPIO_MODE_ANALOG << (MKR_HSE_PIN_PH0 * 2u)) |
                            (MKR_GPIO_MODE_ANALOG << (MKR_HSE_PIN_PH1 * 2u));
    return (moder & ~MKR_HSE_PIN_MODER_MASK) | analog;
}
static inline uint32_t mkr_hse_apply_pupdr(uint32_t pupdr)
{
    return pupdr & ~MKR_HSE_PIN_MODER_MASK;
}
_Static_assert(MKR_GPIO_MODE_ANALOG == 3u, "RM0383 MODER analog is 11");
_Static_assert(MKR_HSE_PIN_MODER_MASK == 0x0000000Fu, "PH0 and PH1 only");
#endif
