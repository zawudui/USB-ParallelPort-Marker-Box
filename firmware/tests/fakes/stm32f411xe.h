#ifndef MKR_TEST_STM32F411XE_H
#define MKR_TEST_STM32F411XE_H
#include <stdint.h>
typedef struct { volatile uint32_t MODER,OTYPER,OSPEEDR,PUPDR,IDR,ODR,BSRR,LCKR,AFR[2]; } GPIO_TypeDef;
typedef struct { volatile uint32_t AHB1ENR; } RCC_TypeDef;
extern GPIO_TypeDef mock_GPIOA,mock_GPIOB,mock_GPIOC,mock_GPIOD,mock_GPIOE;
extern RCC_TypeDef mock_RCC;
#define GPIOA (&mock_GPIOA)
#define GPIOB (&mock_GPIOB)
#define GPIOC (&mock_GPIOC)
#define GPIOD (&mock_GPIOD)
#define GPIOE (&mock_GPIOE)
#define RCC (&mock_RCC)
#define RCC_AHB1ENR_GPIOAEN (UINT32_C(1)<<0)
#define RCC_AHB1ENR_GPIOBEN (UINT32_C(1)<<1)
#define RCC_AHB1ENR_GPIOCEN (UINT32_C(1)<<2)
#define RCC_AHB1ENR_GPIODEN (UINT32_C(1)<<3)
#define RCC_AHB1ENR_GPIOEEN (UINT32_C(1)<<4)
#endif
