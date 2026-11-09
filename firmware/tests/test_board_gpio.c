#include "stm32f411xe.h"
#include "board_gpio.h"
#include <assert.h>
#include <string.h>
GPIO_TypeDef mock_GPIOA,mock_GPIOB,mock_GPIOC,mock_GPIOD,mock_GPIOE;
RCC_TypeDef mock_RCC;
static uint32_t mode(const GPIO_TypeDef*p,unsigned n){return(p->MODER>>(n*2u))&3u;}
static uint32_t pull(const GPIO_TypeDef*p,unsigned n){return(p->PUPDR>>(n*2u))&3u;}
int main(void){
 memset(&mock_GPIOA,0,sizeof(mock_GPIOA));memset(&mock_GPIOB,0,sizeof(mock_GPIOB));memset(&mock_GPIOC,0,sizeof(mock_GPIOC));memset(&mock_GPIOD,0,sizeof(mock_GPIOD));memset(&mock_GPIOE,0,sizeof(mock_GPIOE));memset(&mock_RCC,0,sizeof(mock_RCC));
 mock_GPIOA.MODER=(2u<<22u)|(2u<<24u)|(2u<<26u)|(2u<<28u);
 mock_GPIOA.PUPDR=(3u<<22u)|(3u<<24u)|(1u<<26u)|(2u<<28u);
 uint32_t keep=(3u<<22u)|(3u<<24u)|(3u<<26u)|(3u<<28u), am=mock_GPIOA.MODER&keep,ap=mock_GPIOA.PUPDR&keep;
 mock_GPIOA.OTYPER=UINT32_MAX;mock_GPIOA.OSPEEDR=UINT32_MAX;mock_GPIOB.OTYPER=UINT32_MAX;mock_GPIOB.OSPEEDR=UINT32_MAX;mock_GPIOB.PUPDR=UINT32_MAX;
 board_gpio_init_safe();
 assert(mock_GPIOA.BSRR==(1u<<31u));assert(mock_GPIOB.BSRR==(0x1FFu<<16u));
 for(unsigned n=0;n<=10u;n++){assert(mode(&mock_GPIOA,n)==3u);assert(pull(&mock_GPIOA,n)==0u);}
 assert((mock_GPIOA.MODER&keep)==am&&(mock_GPIOA.PUPDR&keep)==ap);
 assert(mode(&mock_GPIOA,15u)==1u&&pull(&mock_GPIOA,15u)==0u);
 for(unsigned n=0;n<=8u;n++){assert(mode(&mock_GPIOB,n)==1u);assert(pull(&mock_GPIOB,n)==0u);assert((mock_GPIOB.OTYPER&(1u<<n))==0u);assert((mock_GPIOB.OSPEEDR&(3u<<(n*2u)))==0u);}
 for(unsigned n=9u;n<=15u;n++)assert(mode(&mock_GPIOB,n)==3u&&pull(&mock_GPIOB,n)==0u);
 assert(mock_GPIOC.MODER==UINT32_MAX&&mock_GPIOD.MODER==UINT32_MAX&&mock_GPIOE.MODER==UINT32_MAX);
 board_marker_write(0xA5u,true);assert(mock_GPIOB.BSRR==0x005A01A5u);
 board_marker_write(0xFFu,false);assert(mock_GPIOB.BSRR==0x01FF0000u);
 board_iso_power_set(true);assert(mock_GPIOA.BSRR==(1u<<15u));board_iso_power_set(false);assert(mock_GPIOA.BSRR==(1u<<31u));
 board_marker_write(0x0Fu,true);board_iso_power_set(true);board_gpio_force_safe_off();
 assert(mock_GPIOB.BSRR==(0x1FFu<<16u));assert(mock_GPIOA.BSRR==(1u<<31u));
 return 0;
}
