#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS

osThreadId_t thread1_id, thread2_id, thread3_id, thread4_id;

typedef struct{
  int LED;
  int tempo;
} Leds;

void thread1(void *arg){
  uint8_t state = 0;
  Leds Led = *(Leds*)arg;
  while(1){
    state ^= Led.LED;
    LEDWrite(Led.LED, state);
    osDelay(Led.tempo);
  } // while
} // thread1

/*void thread2(void *arg){
  uint8_t state = 0;
  uint32_t tick;
  
  while(1){
    tick = osKernelGetTickCount();   
    state ^= LED2;
    LEDWrite(LED2, state);
    
    osDelayUntil(tick + 100);
  } // while
} // thread2
*/

 void main(void){
  LEDInit(LED1 | LED2 | LED3 | LED4);
  
  Leds Conjunto[4];
  Conjunto[0].LED = LED1;
  Conjunto[0].tempo = 200;
  Conjunto[1].LED = LED2;
  Conjunto[1].tempo = 300;
  Conjunto[2].LED = LED3;
  Conjunto[2].tempo = 500;
  Conjunto[3].LED = LED4;
  Conjunto[3].tempo = 700;

  osKernelInitialize();

  thread1_id = osThreadNew(thread1, (void*)&Conjunto[0], NULL);
  thread2_id = osThreadNew(thread1, (void*)&Conjunto[1], NULL);
  thread3_id = osThreadNew(thread1, (void*)&Conjunto[2], NULL);
  thread4_id = osThreadNew(thread1, (void*)&Conjunto[3], NULL);

  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
} // main
