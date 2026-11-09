#ifndef MKR_STM32F411XE_H
#define MKR_STM32F411XE_H
/*
 * Minimal STM32F411 register map for this firmware.
 * Offsets follow RM0383 and the ST CMSIS device header. This is not a CMSIS
 * distribution and it does not pull in core_cm4.h. Host tests keep using
 * firmware/tests/fakes/stm32f411xe.h via their include path.
 */
#include <stdint.h>
#define __IO volatile
typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t LCKR;
    __IO uint32_t AFR[2];
} GPIO_TypeDef;
typedef struct {
    __IO uint32_t ACR;
    __IO uint32_t KEYR;
    __IO uint32_t OPTKEYR;
    __IO uint32_t SR;
    __IO uint32_t CR;
    __IO uint32_t OPTCR;
    __IO uint32_t OPTCR1;
} FLASH_TypeDef;
typedef struct {
    __IO uint32_t CR;
    __IO uint32_t CSR;
} PWR_TypeDef;
typedef struct {
    __IO uint32_t KR;
    __IO uint32_t PR;
    __IO uint32_t RLR;
    __IO uint32_t SR;
} IWDG_TypeDef;
typedef struct {
    __IO uint32_t TR;
    __IO uint32_t DR;
    __IO uint32_t CR;
    __IO uint32_t ISR;
    __IO uint32_t PRER;
    __IO uint32_t WUTR;
    uint32_t RESERVED0;
    uint32_t RESERVED1;
    uint32_t RESERVED2;
    __IO uint32_t WPR;
} RTC_TypeDef;
typedef struct {
    __IO uint32_t IMR;
    __IO uint32_t EMR;
    __IO uint32_t RTSR;
    __IO uint32_t FTSR;
    __IO uint32_t SWIER;
    __IO uint32_t PR;
} EXTI_TypeDef;
typedef struct {
    __IO uint32_t CR;
    __IO uint32_t PLLCFGR;
    __IO uint32_t CFGR;
    __IO uint32_t CIR;
    __IO uint32_t AHB1RSTR;
    __IO uint32_t AHB2RSTR;
    __IO uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    __IO uint32_t APB1RSTR;
    __IO uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
    __IO uint32_t AHB3ENR;
    uint32_t RESERVED2;
    __IO uint32_t APB1ENR;
    __IO uint32_t APB2ENR;
    uint32_t RESERVED3[2];
    __IO uint32_t AHB1LPENR;
    __IO uint32_t AHB2LPENR;
    __IO uint32_t AHB3LPENR;
    uint32_t RESERVED4;
    __IO uint32_t APB1LPENR;
    __IO uint32_t APB2LPENR;
    uint32_t RESERVED5[2];
    __IO uint32_t BDCR;
    __IO uint32_t CSR;
    uint32_t RESERVED6[2];
    __IO uint32_t SSCGR;
    __IO uint32_t PLLI2SCFGR;
    uint32_t RESERVED7;
    __IO uint32_t DCKCFGR;
} RCC_TypeDef;
typedef struct {
    __IO uint32_t GOTGCTL;
    __IO uint32_t GOTGINT;
    __IO uint32_t GAHBCFG;
    __IO uint32_t GUSBCFG;
    __IO uint32_t GRSTCTL;
    __IO uint32_t GINTSTS;
    __IO uint32_t GINTMSK;
    __IO uint32_t GRXSTSR;
    __IO uint32_t GRXSTSP;
    __IO uint32_t GRXFSIZ;
    __IO uint32_t DIEPTXF0_HNPTXFSIZ;
    __IO uint32_t HNPTXSTS;
    uint32_t Reserved30[2];
    __IO uint32_t GCCFG;
    __IO uint32_t CID;
    uint32_t Reserved40[48];
    __IO uint32_t HPTXFSIZ;
    __IO uint32_t DIEPTXF[15];
} USB_OTG_GlobalTypeDef;
typedef struct {
    __IO uint32_t DCFG;
    __IO uint32_t DCTL;
    __IO uint32_t DSTS;
    uint32_t Reserved0C;
    __IO uint32_t DIEPMSK;
    __IO uint32_t DOEPMSK;
    __IO uint32_t DAINT;
    __IO uint32_t DAINTMSK;
    uint32_t Reserved20;
    uint32_t Reserved9;
    __IO uint32_t DVBUSDIS;
    __IO uint32_t DVBUSPULSE;
    __IO uint32_t DTHRCTL;
    __IO uint32_t DIEPEMPMSK;
    __IO uint32_t DEACHINT;
    __IO uint32_t DEACHMSK;
    uint32_t Reserved40;
    __IO uint32_t DINEP1MSK;
    uint32_t Reserved44[15];
    __IO uint32_t DOUTEP1MSK;
} USB_OTG_DeviceTypeDef;
typedef struct {
    __IO uint32_t DIEPCTL;
    uint32_t Reserved04;
    __IO uint32_t DIEPINT;
    uint32_t Reserved0C;
    __IO uint32_t DIEPTSIZ;
    __IO uint32_t DIEPDMA;
    __IO uint32_t DTXFSTS;
    uint32_t Reserved18;
} USB_OTG_INEndpointTypeDef;
typedef struct {
    __IO uint32_t DOEPCTL;
    uint32_t Reserved04;
    __IO uint32_t DOEPINT;
    uint32_t Reserved0C;
    __IO uint32_t DOEPTSIZ;
    __IO uint32_t DOEPDMA;
    uint32_t Reserved18[2];
} USB_OTG_OUTEndpointTypeDef;
typedef struct {
    __IO uint32_t CTRL;
    __IO uint32_t LOAD;
    __IO uint32_t VAL;
    __IO uint32_t CALIB;
} SysTick_Type;
#define PERIPH_BASE 0x40000000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + 0x00020000UL)
#define AHB2PERIPH_BASE (PERIPH_BASE + 0x10000000UL)
#define APB1PERIPH_BASE PERIPH_BASE
#define GPIOA_BASE (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE (AHB1PERIPH_BASE + 0x0C00UL)
#define GPIOE_BASE (AHB1PERIPH_BASE + 0x1000UL)
#define GPIOH_BASE (AHB1PERIPH_BASE + 0x1C00UL)
#define RCC_BASE (AHB1PERIPH_BASE + 0x3800UL)
#define FLASH_R_BASE (AHB1PERIPH_BASE + 0x3C00UL)
#define APB2PERIPH_BASE (PERIPH_BASE + 0x00010000UL)
#define IWDG_BASE (APB1PERIPH_BASE + 0x3000UL)
#define RTC_BASE (APB1PERIPH_BASE + 0x2800UL)
#define EXTI_BASE (APB2PERIPH_BASE + 0x3C00UL)
#define PWR_BASE (APB1PERIPH_BASE + 0x7000UL)
#define USB_OTG_FS_PERIPH_BASE 0x50000000UL
#define GPIOA ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOH ((GPIO_TypeDef *)GPIOH_BASE)
#define RCC ((RCC_TypeDef *)RCC_BASE)
#define FLASH ((FLASH_TypeDef *)FLASH_R_BASE)
#define PWR ((PWR_TypeDef *)PWR_BASE)
#define IWDG ((IWDG_TypeDef *)IWDG_BASE)
#define RTC ((RTC_TypeDef *)RTC_BASE)
#define EXTI ((EXTI_TypeDef *)EXTI_BASE)
#define SysTick ((SysTick_Type *)0xE000E010UL)
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN (1UL << 2)
#define RCC_AHB1ENR_GPIODEN (1UL << 3)
#define RCC_AHB1ENR_GPIOEEN (1UL << 4)
#define RCC_AHB1ENR_GPIOHEN (1UL << 7)
#define RCC_AHB2ENR_OTGFSEN (1UL << 7)
#define RCC_APB1ENR_TIM2EN (1UL << 0)
#define RCC_APB1ENR_PWREN (1UL << 28)
#define RCC_CSR_LSION (1UL << 0)
#define RCC_CSR_LSIRDY (1UL << 1)
#define RCC_BDCR_RTCSEL_Msk (3UL << 8)
#define RCC_BDCR_RTCSEL_LSI (2UL << 8)
#define RCC_BDCR_RTCEN (1UL << 15)
#define RCC_BDCR_BDRST (1UL << 16)
#define RCC_CFGR_SW (3UL << 0)
#define RCC_CFGR_SWS (3UL << 2)
#define RCC_CFGR_HPRE (0xFUL << 4)
#define RCC_CFGR_PPRE1 (7UL << 10)
#define RCC_CFGR_PPRE2 (7UL << 13)
#define RCC_CIR_CSSF (1UL << 7)
#define RCC_CIR_CSSC (1UL << 23)
#define FLASH_ACR_LATENCY (7UL << 0)
#define FLASH_ACR_LATENCY_2WS 2UL
#define FLASH_ACR_PRFTEN (1UL << 8)
#define FLASH_ACR_ICEN (1UL << 9)
#define FLASH_ACR_DCEN (1UL << 10)
#define PWR_CR_LPDS (1UL << 0)
#define PWR_CR_PDDS (1UL << 1)
#define PWR_CR_DBP (1UL << 8)
#define PWR_CR_FPDS (1UL << 9)
#define PWR_CR_LPLVDS (1UL << 10)
#define PWR_CR_MRLVDS (1UL << 11)
#define PWR_CR_VOS (3UL << 14)
#define PWR_CSR_VOSRDY (1UL << 14)
#define RTC_CR_WUCKSEL_Msk (7UL << 0)
#define RTC_CR_WUTE (1UL << 10)
#define RTC_CR_WUTIE (1UL << 14)
#define RTC_ISR_WUTWF (1UL << 2)
#define RTC_ISR_INIT (1UL << 7)
#define RTC_ISR_WUTF (1UL << 10)
#define MKR_EXTI_USB_WAKEUP (1UL << 18)
#define MKR_EXTI_RTC_WAKEUP (1UL << 22)
#define SysTick_CTRL_ENABLE_Msk (1UL << 0)
#define SysTick_CTRL_TICKINT_Msk (1UL << 1)
#define SysTick_CTRL_CLKSOURCE_Msk (1UL << 2)
#define SysTick_CTRL_COUNTFLAG_Msk (1UL << 16)
#ifndef MKR_USB_BASE
#define MKR_USB_BASE USB_OTG_FS_PERIPH_BASE
#endif
#define USB_OTG_FS ((USB_OTG_GlobalTypeDef *)MKR_USB_BASE)
#define USB_OTG_DEVICE ((USB_OTG_DeviceTypeDef *)(MKR_USB_BASE + 0x800UL))
#define USB_OTG_PCGCCTL (*((__IO uint32_t *)(MKR_USB_BASE + 0xE00UL)))
#define USB_INEP(n) ((USB_OTG_INEndpointTypeDef *)(MKR_USB_BASE + 0x900UL + ((uint32_t)(n) * 0x20UL)))
#define USB_OUTEP(n) ((USB_OTG_OUTEndpointTypeDef *)(MKR_USB_BASE + 0xB00UL + ((uint32_t)(n) * 0x20UL)))
#define USB_DFIFO(n) (*((__IO uint32_t *)(MKR_USB_BASE + 0x1000UL + ((uint32_t)(n) * 0x1000UL))))
#define RTC_WKUP_IRQn 3
#define TIM2_IRQn 28
#define OTG_FS_WKUP_IRQn 42
#define OTG_FS_IRQn 67
typedef struct {
    __IO uint32_t CR1;
    __IO uint32_t CR2;
    __IO uint32_t SMCR;
    __IO uint32_t DIER;
    __IO uint32_t SR;
    __IO uint32_t EGR;
    __IO uint32_t CCMR1;
    __IO uint32_t CCMR2;
    __IO uint32_t CCER;
    __IO uint32_t CNT;
    __IO uint32_t PSC;
    __IO uint32_t ARR;
    uint32_t RESERVED14;
    __IO uint32_t CCR1;
    __IO uint32_t CCR2;
    __IO uint32_t CCR3;
    __IO uint32_t CCR4;
} TIM_TypeDef;
#define TIM2_BASE (APB1PERIPH_BASE + 0x0000UL)
#define TIM2 ((TIM_TypeDef *)TIM2_BASE)
#define TIM_CR1_CEN (1UL << 0)
#define TIM_DIER_CC1IE (1UL << 1)
#define TIM_SR_UIF (1UL << 0)
#define TIM_SR_CC1IF (1UL << 1)
#define TIM_EGR_UG (1UL << 0)
#define TIM_EGR_CC1G (1UL << 1)
#endif
