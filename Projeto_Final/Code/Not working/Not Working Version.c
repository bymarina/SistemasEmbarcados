#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmsis_os2.h"

// includes da biblioteca driverlib
#include "inc/hw_memmap.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "utils/uartstdio.h"
#include "system_TM4C1294.h"

//-----------
// Definições de tamanho de filas de mensagens

#define NUM_OBJETOS_FILA_MENSAGENS 15 //número máximo de mensagens na fila


//-----------
// Estrutura de um objeto da fila de mensangens 
typedef struct {                              
  uint8_t Buffer[6];
  //uint8_t Id;
} OBJ_FILA_MENSAGEM;


//-----------
// Declarações de Id 
osMessageQueueId_t id_Fila_Mensangens_Principal;
osMessageQueueId_t id_Fila_Mensangens_Elevador_Esquerdo;
osMessageQueueId_t id_Fila_Mensangens_Elevador_Direito;
osMessageQueueId_t id_Fila_Mensangens_Elevador_Central;
osMessageQueueId_t id_Fila_Mensangens_Saida; 
              
 
osThreadId_t id_Tarefa_Mensagens_Recebidas;              
osThreadId_t id_Tarefa_Mensagens_Transmitidas; 
osThreadId_t id_Tarefa_Elevador_Esquerdo;
osThreadId_t id_Tarefa_Elevador_Central;
osThreadId_t id_Tarefa_Elevador_Direito;
osThreadId_t id_controle_portas;

osMutexId_t uart_id;

//------------
// Funções 
void Mensagens_Recebidas (void *argument);         
void Mensagens_Transmitidas (void *argument);  

 //----------
// UART definitions
extern void UARTStdioIntHandler(void);

void UARTInit(void){
  // Enable UART0
  SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

  // Initialize the UART for console I/O.
  UARTStdioConfig(0, 115200, SystemCoreClock);

  // Enable the GPIO Peripheral used by the UART.
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
  while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

  // Configure GPIO Pins for UART mode.
  GPIOPinConfigure(GPIO_PA0_U0RX);
  GPIOPinConfigure(GPIO_PA1_U0TX);
  GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
} // UARTInit

void UART0_Handler(void){
  UARTStdioIntHandler();
  osThreadFlagsSet (id_Tarefa_Mensagens_Recebidas, 0x0001);
} // UART0_Handler


//----------- 
// Kernel

void myKernelInfo(void){
  osVersion_t osv;
  char infobuf[16];
  if(osKernelGetInfo(&osv, infobuf, sizeof(infobuf)) == osOK) {
    UARTprintf("Kernel Information: %s\n",   infobuf);
    UARTprintf("Kernel Version    : %d\n",   osv.kernel);
    UARTprintf("Kernel API Version: %d\n\n", osv.api);
    UARTFlushTx(false);
  } // if
} // myKernelInfo

void myKernelState(void){
  UARTprintf("Kernel State: ");
  switch(osKernelGetState()){
    case osKernelInactive:
      UARTprintf("Inactive\n\n");
      break;
    case osKernelReady:
      UARTprintf("Ready\n\n");
      break;
    case osKernelRunning:
      UARTprintf("Running\n\n");
      break;
    case osKernelLocked:
      UARTprintf("Locked\n\n");
      break;
    case osKernelError:
      UARTprintf("Error\n\n");
      break;
  } //switch
  UARTFlushTx(false);
} // myKernelState

//----------
// Estado de Tarefas 

void myThreadState(osThreadId_t thread_id){
  osThreadState_t state;
  state = osThreadGetState(thread_id);
  switch(state){
  case osThreadInactive:
    UARTprintf("Inactive\n");
    break;
  case osThreadReady:
    UARTprintf("Ready\n");
    break;
  case osThreadRunning:
    UARTprintf("Running\n");
    break;
  case osThreadBlocked:
    UARTprintf("Blocked\n");
    break;
  case osThreadTerminated:
    UARTprintf("Terminated\n");
    break;
  case osThreadError:
    UARTprintf("Error\n");
    break;
  } // switch
} // myThreadState

//----------
// Tarefas:

// osRtxIdleThread
__NO_RETURN void osRtxIdleThread(void *argument){
  (void)argument;
  
  while(1){
    //UARTprintf("Idle thread\n");
    asm("wfi");
  } // while
} // osRtxIdleThread

// osRtxTimerThread


__NO_RETURN void Mensagens_Recebidas (void *argument) {
  OBJ_FILA_MENSAGEM msg;  
  osStatus_t status;
  
  char* ponteiro_buffer;
  ponteiro_buffer = (char*)&msg.Buffer;
 
  while (1) {
    osThreadFlagsWait (0x0001, osFlagsWaitAny, osWaitForever);
    UARTgets(ponteiro_buffer, 6);
    osMessageQueuePut(id_Fila_Mensangens_Principal, &msg, 0U, 0U);
    status = osMessageQueueGet(id_Fila_Mensangens_Principal, &msg, NULL, 0U);   // Espera uma mensagem
    if (status == osOK) {
      if (strncmp(msg.Buffer, "i", 1) == 0){
        osThreadFlagsSet (id_controle_portas, 0x0001);
      }
      else if (strncmp(msg.Buffer, "e", 1) == 0){
        osMessageQueuePut(id_Fila_Mensangens_Elevador_Esquerdo, &msg, 0U, 0U);
        osThreadFlagsSet (id_Tarefa_Elevador_Esquerdo, 0x0001);
      }
    }   
    osThreadYield();                                        
  }
}
 
__NO_RETURN void Mensagens_Transmitidas (void *argument) {
  OBJ_FILA_MENSAGEM msg;  
  char* ponteiro_buffer;
  ponteiro_buffer = (char*)&msg.Buffer;
 
  while (1) {
    osThreadFlagsWait (0x0001, osFlagsWaitAny, osWaitForever);
    osMessageQueueGet(id_Fila_Mensangens_Saida, &msg, NULL, 0U); 
    UARTwrite(ponteiro_buffer, 3);
    UARTFlushTx(false);
    osThreadYield();
  }
}

__NO_RETURN void elevador_esquerdo (void *argument){
  OBJ_FILA_MENSAGEM msg_entrada;
  OBJ_FILA_MENSAGEM msg_saida;   
  osStatus_t status; 
  uint8_t andar_solicitado, andar_solicitado_dezena, andar_solicitado_unidade;
  uint8_t i, j, flag;
  uint8_t posicao_dezena_atual, posicao_unidade_atual;
  char sentido;
  char elevador;
  char auxiliar_inicializacao = '0';
  char auxiliar_inicializacao_2 = '0';
  char caracteres_equivalentes [16] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'};
  char conversao_posicao [10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  posicao_dezena_atual = (uint8_t)(auxiliar_inicializacao);
  posicao_unidade_atual = 0 + 0x30;

  while(1){
    osThreadFlagsWait (0x0001, osFlagsWaitAny, osWaitForever);
    status = osMessageQueueGet(id_Fila_Mensangens_Elevador_Esquerdo, &msg_entrada, NULL, 0U);  
    
    elevador = (char)(msg_entrada.Buffer[0]);
    flag = 0;

    if(msg_entrada.Buffer[1] == 'E'){
      andar_solicitado_dezena = (uint8_t)(msg_entrada.Buffer[2]);
      andar_solicitado_unidade = (uint8_t)(msg_entrada.Buffer[3]);
      sentido = msg_entrada.Buffer[4];
      flag = 1;
    }

    else if(msg_entrada.Buffer[1] == 'I'){
      for(i=0;i<15;i++){
        if(msg_entrada.Buffer[2]==caracteres_equivalentes[i]){
          andar_solicitado = i+1;
        }
        flag = 1;
      }
    }   

    else if(msg_entrada.Buffer[1] == '0' || '1' || '2' || '3' || '4' || '5' || '6' || '7' || '8' || '9'){
      flag = 1;
      if(msg_entrada.Buffer[2] != '0' || '1' || '2' || '3' || '4' || '5' || '6' || '7' || '8' || '9'){
        posicao_dezena_atual = (uint8_t)(auxiliar_inicializacao);
        posicao_unidade_atual = (uint8_t)(msg_entrada.Buffer[1]);
      }
      else{
        posicao_dezena_atual = (uint8_t)(msg_entrada.Buffer[1]);
        posicao_unidade_atual = (uint8_t)(msg_entrada.Buffer[2]); 
      }     
    } 

    if(flag == 1){
      if(posicao_unidade_atual == 'F'){
        posicao_unidade_atual == 0 + 0x30;
      }
      if(posicao_dezena_atual < andar_solicitado_dezena){
        msg_saida.Buffer[0] = elevador;
        msg_saida.Buffer[1] = 's';
        msg_saida.Buffer[2] = 13;          
      }
      else if(posicao_dezena_atual > andar_solicitado_dezena){
        msg_saida.Buffer[0] = elevador;
        msg_saida.Buffer[1] = 'd';
        msg_saida.Buffer[2] = 13;
      }
      else if(posicao_dezena_atual == andar_solicitado_dezena){
        if(posicao_unidade_atual > andar_solicitado_unidade){
          msg_saida.Buffer[0] = elevador;
          msg_saida.Buffer[1] = 'd';
          msg_saida.Buffer[2] = 13;
        }
        else if(posicao_unidade_atual < andar_solicitado_unidade){
          msg_saida.Buffer[0] = elevador;
          msg_saida.Buffer[1] = 's';
          msg_saida.Buffer[2] = 13;
        }
        else if(posicao_unidade_atual == andar_solicitado_unidade){
          msg_saida.Buffer[0] = elevador;
          msg_saida.Buffer[1] = 'p';
          msg_saida.Buffer[2] = 13;
        }
    }   
    osMessageQueuePut(id_Fila_Mensangens_Saida, &msg_saida, 0U, 0U);
    osThreadFlagsSet (id_Tarefa_Mensagens_Transmitidas, 0x0001);
    }     
    osThreadYield();    
  }
}
// int luzes
    //mutex

__NO_RETURN void controle_portas(void *argument){
  OBJ_FILA_MENSAGEM msg_portas_esquerdo, msg_portas_central, msg_portas_direito;

  msg_portas_esquerdo.Buffer[0] = 'e';
  msg_portas_esquerdo.Buffer[1] = 'f';
  msg_portas_esquerdo.Buffer[2] = 13; 

  msg_portas_central.Buffer[0] = 'c';
  msg_portas_central.Buffer[1] = 'f';
  msg_portas_central.Buffer[2] = 13; 

  msg_portas_direito.Buffer[0] = 'd';
  msg_portas_direito.Buffer[1] = 'f';
  msg_portas_direito.Buffer[2] = 13; 

  while(1){
    osThreadFlagsWait(0x0001, osFlagsWaitAny, osWaitForever);
    osMessageQueuePut(id_Fila_Mensangens_Saida, &msg_portas_esquerdo, 0U, 0U);
    osMessageQueuePut(id_Fila_Mensangens_Saida, &msg_portas_central, 0U, 0U);
    osMessageQueuePut(id_Fila_Mensangens_Saida, &msg_portas_direito, 0U, 0U);
    osThreadFlagsSet (id_Tarefa_Mensagens_Transmitidas, 0x0001); 
    osThreadYield();
  }
}

//------------

 void main(void){
    
    UARTInit();
    myKernelInfo();
    myKernelState();
  
    if(osKernelGetState() == osKernelInactive)
      osKernelInitialize();

    myKernelState();   

    //Tarefas
    id_Tarefa_Mensagens_Recebidas = osThreadNew(Mensagens_Recebidas, NULL, NULL);
    id_Tarefa_Mensagens_Transmitidas = osThreadNew(Mensagens_Transmitidas, NULL, NULL);
    id_Tarefa_Elevador_Esquerdo = osThreadNew(elevador_esquerdo, NULL, NULL);
    //id_Tarefa_Elevador_Central = osThreadNew(elevador_central, NULL, NULL);
    //id_Tarefa_Elevador_Direito = osThreadNew(elevador_direito, NULL, NULL);
    id_controle_portas = osThreadNew(controle_portas, NULL, NULL);
    

    //Listas de Mensagem
    id_Fila_Mensangens_Principal = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);
    id_Fila_Mensangens_Elevador_Esquerdo = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);
    //id_Fila_Mensangens_Elevador_Central = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);
    //id_Fila_Mensangens_Elevador_Direito = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);
    id_Fila_Mensangens_Saida = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);

    //Mutexes e Semáforos
    uart_id = osMutexNew(NULL);
    
    if(osKernelGetState() == osKernelReady)
    osKernelStart();

    while(1);
}
