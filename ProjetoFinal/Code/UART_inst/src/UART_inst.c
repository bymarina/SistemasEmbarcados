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
// Defini��es de tamanho de filas de mensagens

#define NUM_OBJETOS_FILA_MENSAGENS 15 //n�mero m�ximo de mensagens na fila


//-----------
// Estrutura de um objeto da fila de mensangens 
typedef struct {                              
  uint8_t Buffer[6];
  //uint8_t Id;
} OBJ_FILA_MENSAGEM;


//-----------
// Declara��es de Id 
osMessageQueueId_t id_Fila_Mensangens_Principal;
osMessageQueueId_t id_Fila_Mensangens_Elevador_Esquerdo;
osMessageQueueId_t id_Fila_Mensangens_Saida; 
              
 
osThreadId_t id_Tarefa_Mensagens_Recebidas;              
osThreadId_t id_Tarefa_Mensagens_Transmitidas; 
osThreadId_t id_Tarefa_Elevador_Esquerdo;
osThreadId_t id_controle_portas;


//------------
// Fun��es 
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
      else if ( (strncmp(msg.Buffer, "e", 1) == 0) && (strncmp(msg.Buffer, "eA", 2) != 0) && (strncmp(msg.Buffer, "eF", 2) != 0) ){
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
  char caracteres_equivalentes [16] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p'};
  char conversao_posicao [10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
  posicao_dezena_atual = 0 + 0x30;
  posicao_unidade_atual = 0 + 0x30;

  while(1){
    osThreadFlagsWait (0x0001, osFlagsWaitAny, osWaitForever);
    status = osMessageQueueGet(id_Fila_Mensangens_Elevador_Esquerdo, &msg_entrada, NULL, 0U);      

    if(msg_entrada.Buffer[1] == 'E'){
      andar_solicitado_dezena = (uint8_t)(msg_entrada.Buffer[2]);
      andar_solicitado_unidade = (uint8_t)(msg_entrada.Buffer[3]);
      sentido = msg_entrada.Buffer[4];
    }

    else if(msg_entrada.Buffer[1] == 'I'){
        if(msg_entrada.Buffer[2] == 'a'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'0';}
        else if(msg_entrada.Buffer[2] == 'b'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'1';}
        else if(msg_entrada.Buffer[2] == 'c'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'2';}
        else if(msg_entrada.Buffer[2] == 'd'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'3';}
        else if(msg_entrada.Buffer[2] == 'e'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'4';}
        else if(msg_entrada.Buffer[2] == 'f'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'5';}
        else if(msg_entrada.Buffer[2] == 'g'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'6';}
        else if(msg_entrada.Buffer[2] == 'h'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'7';}
        else if(msg_entrada.Buffer[2] == 'i'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'8';}
        else if(msg_entrada.Buffer[2] == 'j'){andar_solicitado_dezena=(uint8_t)'0'; andar_solicitado_unidade = (uint8_t)'9';}
        else if(msg_entrada.Buffer[2] == 'k'){andar_solicitado_dezena=(uint8_t)'1'; andar_solicitado_unidade = (uint8_t)'0';}
        else if(msg_entrada.Buffer[2] == 'l'){andar_solicitado_dezena=(uint8_t)'1'; andar_solicitado_unidade = (uint8_t)'1';}
        else if(msg_entrada.Buffer[2] == 'm'){andar_solicitado_dezena=(uint8_t)'1'; andar_solicitado_unidade = (uint8_t)'2';}
        else if(msg_entrada.Buffer[2] == 'n'){andar_solicitado_dezena=(uint8_t)'1'; andar_solicitado_unidade = (uint8_t)'3';}
        else if(msg_entrada.Buffer[2] == 'o'){andar_solicitado_dezena=(uint8_t)'1'; andar_solicitado_unidade = (uint8_t)'4';}
        else if(msg_entrada.Buffer[2] == 'p'){andar_solicitado_dezena=(uint8_t)'1'; andar_solicitado_unidade = (uint8_t)'5';}
    }   

    else if(msg_entrada.Buffer[1] == '0' || '1' || '2' || '3' || '4' || '5' || '6' || '7' || '8' || '9'){
      if(msg_entrada.Buffer[2] != '0' || '1' || '2' || '3' || '4' || '5' || '6' || '7' || '8' || '9'){
        posicao_dezena_atual = (uint8_t)('0');
        posicao_unidade_atual = (uint8_t)(msg_entrada.Buffer[1]);
      }
      else{
        posicao_dezena_atual = (uint8_t)(msg_entrada.Buffer[1]);
        posicao_unidade_atual = (uint8_t)(msg_entrada.Buffer[2]); 
      }     
    } 

    if(posicao_dezena_atual < andar_solicitado_dezena){
        msg_saida.Buffer[0] = 'e';
        msg_saida.Buffer[1] = 's';
        msg_saida.Buffer[2] = 13;          
      }
    else if(posicao_dezena_atual > andar_solicitado_dezena){
        msg_saida.Buffer[0] = 'e';
        msg_saida.Buffer[1] = 'd';
        msg_saida.Buffer[2] = 13;
      }
    else if(posicao_dezena_atual == andar_solicitado_dezena){
        if(posicao_unidade_atual > andar_solicitado_unidade){
            msg_saida.Buffer[0] = 'e';
            msg_saida.Buffer[1] = 'd';
            msg_saida.Buffer[2] = 13;
        }
        else if(posicao_unidade_atual < andar_solicitado_unidade){
            msg_saida.Buffer[0] = 'e';
            msg_saida.Buffer[1] = 's';
            msg_saida.Buffer[2] = 13;
        }
        else if(posicao_unidade_atual == andar_solicitado_unidade){
            msg_saida.Buffer[0] = 'e';
            msg_saida.Buffer[1] = 'p';
            msg_saida.Buffer[2] = 13;
        }
    }   
    osMessageQueuePut(id_Fila_Mensangens_Saida, &msg_saida, 0U, 0U);
    osThreadFlagsSet (id_Tarefa_Mensagens_Transmitidas, 0x0001);
    osThreadYield();
    }      
}


__NO_RETURN void controle_portas(void *argument){
  OBJ_FILA_MENSAGEM msg_portas_esquerdo, msg_portas_central, msg_portas_direito;

  msg_portas_esquerdo.Buffer[0] = 'e';
  msg_portas_esquerdo.Buffer[1] = 'f';
  msg_portas_esquerdo.Buffer[2] = 13;

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
    id_controle_portas = osThreadNew(controle_portas, NULL, NULL);
    

    //Listas de Mensagem
    id_Fila_Mensangens_Principal = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);
    id_Fila_Mensangens_Elevador_Esquerdo = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);
    id_Fila_Mensangens_Saida = osMessageQueueNew(NUM_OBJETOS_FILA_MENSAGENS, sizeof(OBJ_FILA_MENSAGEM), NULL);

    
    if(osKernelGetState() == osKernelReady)
    osKernelStart();

    while(1);
}
