#include "system_tm4c1294.h" // CMSIS-Core
#include "driverleds.h" // device drivers
#include "cmsis_os2.h" // CMSIS-RTOS
#include <stdbool.h>
#include "driverbuttons.h"
#include "inc/hw_nvic.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

osThreadId_t produtor_id, consumidor_id;
osSemaphoreId_t vazio_id, cheio_id;

#define BUFFER_SIZE 8
uint8_t buffer[BUFFER_SIZE];

void GPIOJ_Handler(void){
  ButtonIntClear(USW1 | USW2);
  osThreadFlagsSet (produtor_id, 0x0010); //set thread flag
}

void produtor(void *arg){
  uint8_t index_i = 0, count = 0;

  while(1){
    osThreadFlagsWait (0x0010, osFlagsWaitAny, osWaitForever); // wait for thread flag
    osSemaphoreAcquire(vazio_id, osWaitForever); // is there space avaliable?
    buffer[index_i] =  count; // put in buffer
    osSemaphoreRelease(cheio_id); // signal one less space
    
    index_i++; // increases buffering index
    if(index_i >= BUFFER_SIZE)
      index_i = 0;
    
    count++;
    count &= 0x0F; // produce new info
    osDelay(500);
  }
} 

void consumidor(void *arg){
  uint8_t index_o = 0, state;
  
  while(1){
    osSemaphoreAcquire(cheio_id, osWaitForever); // is there data available?
    state = buffer[index_o]; // remove from buffer
    osSemaphoreRelease(vazio_id); // signal an extra space
    
    index_o++;
    if(index_o >= BUFFER_SIZE) // increases buffer withdrawal index
      index_o = 0;
    
    LEDWrite(LED4 | LED3 | LED2 | LED1, state);
    osDelay(500);
  }
}

void main(void){
  SystemInit();
  LEDInit(LED4 | LED3 | LED2 | LED1);

  osKernelInitialize();
  
  ButtonInit(USW1 | USW2);
  ButtonIntEnable(USW1 | USW2);  

  produtor_id = osThreadNew(produtor, NULL, NULL);
  consumidor_id = osThreadNew(consumidor, NULL, NULL);

  vazio_id = osSemaphoreNew(BUFFER_SIZE, BUFFER_SIZE, NULL);
  cheio_id = osSemaphoreNew(BUFFER_SIZE, 0, NULL);
  
  if(osKernelGetState() == osKernelReady)
    osKernelStart();

  while(1);
}