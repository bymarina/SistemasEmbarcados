#include <stdint.h>
#include <stdbool.h>
// includes da biblioteca driverlib
#include "driverlib/systick.h"
#include "driverleds.h" // Projects/drivers

// MEF com apenas 2 estados e 1 evento temporal que alterna entre eles
// Seleção por evento

typedef enum {E000, E001, E011, E010, E110, E111, E101, E100 } state_t;

volatile uint8_t Evento = 0;

void SysTick_Handler(void){
  Evento = 1;
} // SysTick_Handler

void main(void){
  static state_t Estado = E000; // estado inicial da MEF
  
  LEDInit(LED1);
  LEDInit(LED2);
  LEDInit(LED3);
  
  SysTickPeriodSet(12000000); // f = 1Hz para clock = 24MHz
  SysTickIntEnable();
  SysTickEnable();

  while(1){
    if(Evento){
      Evento = 0;
      switch(Estado){
        case E000:
          LEDOff(LED1);
          LEDOff(LED2);
          LEDOff(LED3);
          Estado = E001;
          break;
        case E001:
          LEDOff(LED1);
          LEDOff(LED2);
          LEDOn(LED3);
          Estado = E011;
          break;
       case E011:
          LEDOff(LED1);
          LEDOn(LED2);
          LEDOn(LED3);
          Estado = E010;
           break;
       case E010:
          LEDOff(LED1);
          LEDOn(LED2);
          LEDOff(LED3);
          Estado = E110;
           break;
       case E110:
          LEDOn(LED1);
          LEDOn(LED2);
          LEDOff(LED3);
          Estado = E111;
          break;
       case E111:
          LEDOn(LED1);
          LEDOn(LED2);
          LEDOn(LED3);
          Estado = E101;
          break;
       case E101:
          LEDOn(LED1);
          LEDOff(LED2);
          LEDOn(LED3);
          Estado = E100;
          break;
       case E100:
          LEDOn(LED1);
          LEDOff(LED2);
          LEDOff(LED3);
          Estado = E000;
          break;
          
      } // switch
    } // if
  } // while
} // main
