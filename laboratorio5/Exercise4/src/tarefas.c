#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS

osThreadId_t thread1_id, thread2_id, thread3_id, thread4_id; 
osMutexId_t mutex1_id, mutex2_id;  

typedef struct{
  int LED;
  int tempo;
} Leds;

const osMutexAttr_t Mutex1_attributes = {
  "Leds Mutex1", 0, NULL, 0
};

const osMutexAttr_t Mutex2_attributes = {
  "Leds Mutex2", 0, NULL, 0
};

void create_mutexes(void){
  mutex1_id = osMutexNew(&Mutex1_attributes);
  mutex2_id = osMutexNew(&Mutex2_attributes);       
}

void thread1(void *arg){
  uint8_t state = 0;
  uint32_t tick;
  Leds Led = *(Leds*)arg;

  while(1){
    osMutexAcquire(mutex1_id, osWaitForever);
    tick = osKernelGetTickCount();   
    state ^= Led.LED;
    osMutexAcquire(mutex2_id, osWaitForever);
    LEDWrite(Led.LED, state);    
    osMutexRelease(mutex2_id);
    osDelayUntil(tick + 100);
    osMutexRelease(mutex1_id);
  }
} 

void thread2(void *arg){
  uint8_t state = 0;
  uint32_t tick;
  Leds Led = *(Leds*)arg;

  while(1){
    osMutexAcquire(mutex2_id, osWaitForever);
    tick = osKernelGetTickCount();   
    state ^= Led.LED;
    osMutexAcquire(mutex1_id, osWaitForever);
    LEDWrite(Led.LED, state);    
    osMutexRelease(mutex1_id);
    osDelayUntil(tick + 101);
    osMutexRelease(mutex2_id);
  }
}

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
  create_mutexes();

  thread1_id = osThreadNew(thread1, (void*)&Conjunto[0], NULL);
  thread2_id = osThreadNew(thread2, (void*)&Conjunto[1], NULL);
  //thread3_id = osThreadNew(thread, (void*)&Conjunto[2], NULL);
  //thread4_id = osThreadNew(thread, (void*)&Conjunto[3], NULL);

  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
}
