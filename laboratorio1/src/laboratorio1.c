#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"

#define __SYSTEM_CLOCK    (24000000UL);

uint8_t LED_D4 = 0;

void delay_1sec(void){
  for(unsigned long i=0; i<11000000; i++);
}

void main(void){
  
  SysTickPeriodSet(12000000); // f = 1Hz para clock = 24MHz
  
  //inicialização do LED 4
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));  
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_0);
  GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
  
  while(1){
    delay_1sec();
    LED_D4 ^= GPIO_PIN_0;
    GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, LED_D4);
  }

}