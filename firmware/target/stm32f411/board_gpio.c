#include "board_gpio.h"
#include "stm32f411xe.h"
#define BOARD_OUTPUT_MASK (MKR_DATA_MASK | MKR_READY_MASK)
static void set_pin_mode(GPIO_TypeDef *port,uint32_t pin,uint32_t value){const uint32_t shift=pin*2u;const uint32_t mask=UINT32_C(3)<<shift;port->MODER=(port->MODER&~mask)|(value<<shift);}
static void set_pin_no_pull(GPIO_TypeDef *port,uint32_t pin){port->PUPDR&=~(UINT32_C(3)<<(pin*2u));}
void board_gpio_init_safe(void){
 const uint32_t gpio_ab=RCC_AHB1ENR_GPIOAEN|RCC_AHB1ENR_GPIOBEN;
 const uint32_t gpio_cde=RCC_AHB1ENR_GPIOCEN|RCC_AHB1ENR_GPIODEN|RCC_AHB1ENR_GPIOEEN;
 RCC->AHB1ENR|=gpio_ab|gpio_cde;(void)RCC->AHB1ENR;
 /* Preload low while pins are still in their reset modes. */
 GPIOA->BSRR=UINT32_C(1)<<(MKR_PIN_ISO_PWR_EN+16u);GPIOB->BSRR=BOARD_OUTPUT_MASK<<16u;
 GPIOA->OTYPER&=~(UINT32_C(1)<<MKR_PIN_ISO_PWR_EN);GPIOA->OSPEEDR&=~(UINT32_C(3)<<(MKR_PIN_ISO_PWR_EN*2u));set_pin_no_pull(GPIOA,MKR_PIN_ISO_PWR_EN);
 GPIOB->OTYPER&=~BOARD_OUTPUT_MASK;
 for(uint32_t pin=0u;pin<=MKR_PIN_READY;pin++){GPIOB->OSPEEDR&=~(UINT32_C(3)<<(pin*2u));set_pin_no_pull(GPIOB,pin);}
 /* Only switch to output after the output latches and electrical settings are safe. */
 set_pin_mode(GPIOA,MKR_PIN_ISO_PWR_EN,1u);for(uint32_t pin=0u;pin<=MKR_PIN_READY;pin++)set_pin_mode(GPIOB,pin,1u);
 /* Unused PA0-PA10: analog. Preserve USB PA11/PA12 and SWD PA13/PA14. */
 for(uint32_t pin=0u;pin<=10u;pin++){set_pin_mode(GPIOA,pin,3u);set_pin_no_pull(GPIOA,pin);}
 /* Unused/unbonded PB9-PB15: analog, no pull. */
 for(uint32_t pin=9u;pin<=15u;pin++){set_pin_mode(GPIOB,pin,3u);set_pin_no_pull(GPIOB,pin);}
 /* PC, PD and PE are unused on this board. Configure before gating their clocks. */
 GPIOC->MODER=UINT32_MAX;GPIOC->PUPDR=0u;GPIOD->MODER=UINT32_MAX;GPIOD->PUPDR=0u;GPIOE->MODER=UINT32_MAX;GPIOE->PUPDR=0u;
 RCC->AHB1ENR&=~gpio_cde;(void)RCC->AHB1ENR;
}
/*
 * Safety semantics: ready=false forces D0-D7 and PB8 READY low together,
 * regardless of requested code. To publish a nonzero code, call with
 * ready=true; PB0-PB8 are updated in one BSRR write.
 */
void board_marker_write(uint8_t code,bool ready){
 const uint32_t set_mask=ready?((uint32_t)code|MKR_READY_MASK):0u;
 const uint32_t reset_mask=(BOARD_OUTPUT_MASK&~set_mask)<<16u;
 GPIOB->BSRR=set_mask|reset_mask;
}
void board_iso_power_set(bool enabled){GPIOA->BSRR=UINT32_C(1)<<(enabled?MKR_PIN_ISO_PWR_EN:MKR_PIN_ISO_PWR_EN+16u);}
/* Same end state as a latched fault: data/READY low, then PA15 low. Idempotent. */
void board_gpio_force_safe_off(void){GPIOB->BSRR=BOARD_OUTPUT_MASK<<16u;GPIOA->BSRR=UINT32_C(1)<<(MKR_PIN_ISO_PWR_EN+16u);}
