#ifndef MKR_STM32F411XE_H
#define MKR_STM32F411XE_H
/*
 * Host stand-in for the target register header. Not ST CMSIS.
 * Only the USB FS device controller test includes this, via its include path.
 * Layout of the USB structs matches the fields the driver names. Bases are RAM.
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
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
} RCC_TypeDef;
typedef struct {
    __IO uint32_t IMR;
    __IO uint32_t EMR;
    __IO uint32_t RTSR;
    __IO uint32_t FTSR;
    __IO uint32_t SWIER;
    __IO uint32_t PR;
} EXTI_TypeDef;
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
extern uint8_t mkr_host_usb_mem[0x5000];
extern GPIO_TypeDef mkr_host_gpioa;
extern RCC_TypeDef mkr_host_rcc;
extern EXTI_TypeDef mkr_host_exti;
#define GPIOA (&mkr_host_gpioa)
#define RCC (&mkr_host_rcc)
#define EXTI (&mkr_host_exti)
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define RCC_AHB2ENR_OTGFSEN (1UL << 7)
#define MKR_EXTI_USB_WAKEUP (1UL << 18)
#define OTG_FS_WKUP_IRQn 42
#define OTG_FS_IRQn 67
#define MKR_USB_BASE ((uintptr_t)mkr_host_usb_mem)
#define USB_OTG_FS ((USB_OTG_GlobalTypeDef *)MKR_USB_BASE)
#define USB_OTG_DEVICE ((USB_OTG_DeviceTypeDef *)(MKR_USB_BASE + 0x800UL))
#define USB_OTG_PCGCCTL (*((__IO uint32_t *)(MKR_USB_BASE + 0xE00UL)))
#define USB_INEP(n) ((USB_OTG_INEndpointTypeDef *)(MKR_USB_BASE + 0x900UL + ((uint32_t)(n) * 0x20UL)))
#define USB_OUTEP(n) ((USB_OTG_OUTEndpointTypeDef *)(MKR_USB_BASE + 0xB00UL + ((uint32_t)(n) * 0x20UL)))
#define USB_DFIFO(n) (*((__IO uint32_t *)(MKR_USB_BASE + 0x1000UL + ((uint32_t)(n) * 0x1000UL))))
#endif
