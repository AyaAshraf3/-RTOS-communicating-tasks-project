/*
 * This file is part of the µOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "diag/trace.h"

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include <inttypes.h>


#define CCM_RAM __attribute__((section(".ccmram")))

// ----------------------------------------------------------------------------

#include "led.h"

#define BLINK_PORT_NUMBER         (3)
#define BLINK_PIN_NUMBER_GREEN    (12)
#define BLINK_PIN_NUMBER_ORANGE   (13)
#define BLINK_PIN_NUMBER_RED      (14)
#define BLINK_PIN_NUMBER_BLUE     (15)
#define BLINK_ACTIVE_LOW          (false)




struct led blinkLeds[4];

// ----------------------------------------------------------------------------
/*-----------------------------------------------------------*/

/*
 * The LED timer callback function.  This does nothing but switch the red LED
 * off.
 */

static void prvOneShotTimerCallback( TimerHandle_t xTimer );
static void prvAutoReloadTimerCallback( TimerHandle_t xTimer );

/*-----------------------------------------------------------*/

/* The LED software timer.  This uses vLEDTimerCallback() as its callback
 * function.
 */
static TimerHandle_t xTimer1 = NULL;
static TimerHandle_t xTimer2 = NULL;
BaseType_t xTimer1Started, xTimer2Started;

/*-----------------------------------------------------------*/
// ----------------------------------------------------------------------------
//
// Semihosting STM32F4 empty sample (trace via DEBUG).
//
// Trace support is enabled by adding the TRACE macro definition.
// By default the trace messages are forwarded to the DEBUG output,
// but can be rerouted to any device or completely suppressed, by
// changing the definitions required in system/src/diag/trace-impl.c
// (currently OS_USE_TRACE_ITM, OS_USE_TRACE_SEMIHOSTING_DEBUG/_STDOUT).
//

// ----- main() ---------------------------------------------------------------

// Sample pragmas to cope with warnings. Please note the related line at
// the end of this function, used to pop the compiler diagnostics status.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"




//Deceleration for statistics counters

int Blocked_msgs_1;
int Blocked_msgs_2;
int Blocked_msgs_3;
int Transmitted_msgs_1;
int Transmitted_msgs_2;
int Transmitted_msgs_3;
int Received_msgs=0;

int Sender_TIMER_PERIOD_1;
int Sender_TIMER_PERIOD_2;
int Sender_TIMER_PERIOD_3;
int Receiver_TIMER_PERIOD;



//global Queue

QueueHandle_t testQueue;



//global semaphores handlers

SemaphoreHandle_t SenderTask_semphr_1;
SemaphoreHandle_t SenderTask_semphr_2;
SemaphoreHandle_t SenderTask_semphr_3;
SemaphoreHandle_t ReceiverTask_semphr;

//global timers handlers

TimerHandle_t autoReloadTimer1;
TimerHandle_t autoReloadTimer2;
TimerHandle_t autoReloadTimer3;
TimerHandle_t autoReloadTimer4;

//parameter to sweep on 6 iterations
int index=0;

//the lower and upper bounds for Uniform Distribution Random function generated for the Timers Periods (in the 6 iterations)

int lower [6]={50,80,110,140,170,200};
int upper [6]={150,200,250,300,350,400};



//the TimerCallback functions where the semaphores are released so that Tasks can take it and perform (as the initial state of semaphores used is 'already taken')

static void Sender_1_TimerCallback( TimerHandle_t xTimer )
{
	 if (Received_msgs  >= 1000) {
	            Reset();

	  }

	 BaseType_t xHigherPriorityTaskWoken=pdFALSE;
	 xSemaphoreGiveFromISR(SenderTask_semphr_1,&xHigherPriorityTaskWoken);

	 Sender_TIMER_PERIOD_1=pdMS_TO_TICKS(generateRandomPeriod(lower[index],upper[index]));
	 xTimerChangePeriod(xTimer,Sender_TIMER_PERIOD_1 , 0);


}

static void Sender_2_TimerCallback( TimerHandle_t xTimer )
{
	 if (Received_msgs  >= 1000) {
	            Reset();
	  }
	 BaseType_t xHigherPriorityTaskWoken=pdFALSE;
	 xSemaphoreGiveFromISR(SenderTask_semphr_2,&xHigherPriorityTaskWoken);

	 Sender_TIMER_PERIOD_2=pdMS_TO_TICKS(generateRandomPeriod(lower[index],upper[index]));
	 xTimerChangePeriod(xTimer,Sender_TIMER_PERIOD_2, 0);


}

static void Sender_3_TimerCallback( TimerHandle_t xTimer )
{
	 if (Received_msgs  >= 1000) {
		            Reset();
		  }
	 BaseType_t xHigherPriorityTaskWoken=pdFALSE;
	 xSemaphoreGiveFromISR(SenderTask_semphr_3,&xHigherPriorityTaskWoken);

	 Sender_TIMER_PERIOD_3=pdMS_TO_TICKS(generateRandomPeriod(lower[index],upper[index]));
	 xTimerChangePeriod(xTimer,Sender_TIMER_PERIOD_3, 0);


}

static void Receiver_TimerCallback( TimerHandle_t xTimer )
{
	 if (Received_msgs  >= 1000) {
		            Reset();
		  }
	xSemaphoreGive(ReceiverTask_semphr);


}

//Sender Tasks

void SenderTask_1(void *pvParameters){
	BaseType_t xStatus;
	char ValueToSend[20];

	while(1){


	xSemaphoreTake(SenderTask_semphr_1,portMAX_DELAY);
	TickType_t currentTickCount = xTaskGetTickCount();
	// Convert the tick count to a string
	sprintf(ValueToSend, "Time is %lu", currentTickCount);
	xStatus = xQueueSend(testQueue,&ValueToSend,0);
	if (xStatus != pdPASS){ //sending failed
		Blocked_msgs_1++;
	}
	else{

		Transmitted_msgs_1++;
	}
	}
}

void SenderTask_2(void *pvParameters){
	BaseType_t xStatus;
	char ValueToSend[100];

	while(1){


	xSemaphoreTake(SenderTask_semphr_2,portMAX_DELAY);
	TickType_t currentTickCount = xTaskGetTickCount();
	// Convert the tick count to a string
	sprintf(ValueToSend, "Time is %lu", currentTickCount);
	xStatus = xQueueSend(testQueue,&ValueToSend,0);
	if (xStatus != pdPASS){ //sending failed
		Blocked_msgs_2++;
	}
	else{

		Transmitted_msgs_2++;
	}
	}
}

void SenderTask_3(void *pvParameters){
	BaseType_t xStatus;
	char ValueToSend[100];

	while(1){
	xSemaphoreTake(SenderTask_semphr_3,portMAX_DELAY);
	TickType_t currentTickCount = xTaskGetTickCount();
	// Convert the tick count to a string
		sprintf(ValueToSend, "Time is %lu", currentTickCount);

	xStatus = xQueueSend(testQueue,&ValueToSend,0);
	if (xStatus != pdPASS){ //sending failed
		Blocked_msgs_3++;
	}
	else{

		Transmitted_msgs_3++;
	}
	}
}

//Receiver Task

static void ReceiverTask( void *parameters )
{
	char receivedValue[20];
  BaseType_t status;
  while(1){
	  xSemaphoreTake(ReceiverTask_semphr,portMAX_DELAY);
  status = xQueueReceive( testQueue, &receivedValue,0);

  if( status == pdPASS ){
      Received_msgs +=1;
  }

  }

}

//function to generate uniform distributed random number

int generateRandomPeriod(int minperiod,int maxperiod){
	int range=maxperiod - minperiod;
	int randomperiod=(rand() %range)+minperiod;
	return randomperiod;
}

//function to reset the system which is called at first in main and in the CallBack functions at the end of each iteration.

void Reset(){

    if (Received_msgs==0){ //first run

    	Blocked_msgs_1=0;
    	Blocked_msgs_2=0;
    	Blocked_msgs_3=0;
    	Transmitted_msgs_1=0;
    	Transmitted_msgs_2=0;
    	Transmitted_msgs_3=0;
    	xQueueReset(testQueue);

    }


    else{

	printf("iteration number :%d\n",index+1);



	printf("Total successfully sent :%d\n",Transmitted_msgs_1+Transmitted_msgs_2+Transmitted_msgs_3);

	printf("Total Blocked messages :%d\n",Blocked_msgs_1+Blocked_msgs_2+Blocked_msgs_3);

	printf("Statistics :\n");
	printf("sender task 1(lower priority):\n");
	printf("number of successfully sent messages: %d\n", Transmitted_msgs_1);
	printf("number of blocked messages: %d\n", Blocked_msgs_1);


	printf("sender task 2(lower priority):\n");
	printf("number of successfully sent messages: %d\n", Transmitted_msgs_2);
	printf("number of blocked messages: %d\n", Blocked_msgs_2);


	printf("sender task 3(higher priority):\n");
	printf("number of successfully sent messages: %d\n", Transmitted_msgs_3);
	printf("number of blocked messages: %d\n", Blocked_msgs_3);
	printf("\n");
	printf("\n");


	Blocked_msgs_1=0;
	Blocked_msgs_2=0;
	Blocked_msgs_3=0;
	Transmitted_msgs_1=0;
	Transmitted_msgs_2=0;
	Transmitted_msgs_3=0;
	Received_msgs=0;



	xQueueReset(testQueue);

	index++;
    }

	if (index == 6){ //program reached END

		//Timers Destruction
		xTimerDelete( autoReloadTimer1,0);
		xTimerDelete( autoReloadTimer2,0);
		xTimerDelete( autoReloadTimer3,0);
		xTimerDelete( autoReloadTimer4,0);

		printf("Game Over\n");

		//Quit(terminate) the program
		exit(0);
	}


}





int
main(int argc, char* argv[])
{


	srand(time(0));

	testQueue=xQueueCreate(10, sizeof(int8_t*)*5); //Queue Creation

	//Reset program

    Reset();

    //Timers Creation

	Sender_TIMER_PERIOD_1= pdMS_TO_TICKS(generateRandomPeriod(lower[index],upper[index]));
	Sender_TIMER_PERIOD_2= pdMS_TO_TICKS(generateRandomPeriod(lower[index],upper[index]));
	Sender_TIMER_PERIOD_3= pdMS_TO_TICKS(generateRandomPeriod(lower[index],upper[index]));
	Receiver_TIMER_PERIOD =pdMS_TO_TICKS( 100 );



    //Semaphores Creation

	SenderTask_semphr_1= xSemaphoreCreateBinary();
	SenderTask_semphr_2= xSemaphoreCreateBinary();
	SenderTask_semphr_3= xSemaphoreCreateBinary();
	ReceiverTask_semphr= xSemaphoreCreateBinary();



    //Timer 1 created

	autoReloadTimer1 = xTimerCreate("SenderTask1",Sender_TIMER_PERIOD_1,pdTRUE,0,Sender_1_TimerCallback );
	if( autoReloadTimer1 != NULL )
	xTimerStart(autoReloadTimer1, 0 );
	/* 0 means start now */

	//Sender Task 1 creation (priority '1')
    xTaskCreate(SenderTask_1,"SenderTask1",1000,NULL,1,NULL);

    //Timer 2 created

	autoReloadTimer2 = xTimerCreate("SenderTask2",Sender_TIMER_PERIOD_2,pdTRUE,0,Sender_2_TimerCallback );
	if( autoReloadTimer2 != NULL )
    xTimerStart(autoReloadTimer2, 0 );
	/* 0 means start now */

    //Sender Task 2 creation (priority '1')
    xTaskCreate(SenderTask_2,"SenderTask2",1000,NULL,1,NULL);

    //Timer 3 created

	autoReloadTimer3 = xTimerCreate("SenderTask3",Sender_TIMER_PERIOD_3,pdTRUE,0,Sender_3_TimerCallback );
	if( autoReloadTimer3 != NULL )
	xTimerStart(autoReloadTimer3, 0 );
	/* 0 means start now */

    //Sender Task 3 creation (priority '2')
     xTaskCreate(SenderTask_3,"SenderTask3",1000,NULL,2,NULL);

    //Timer 4 created

	autoReloadTimer4 = xTimerCreate("ReceiverTask",Receiver_TIMER_PERIOD,pdTRUE,0,Receiver_TimerCallback );
	if( autoReloadTimer4 != NULL )
	xTimerStart(autoReloadTimer4, 0 );
	/* 0 means start now */

    //Receiver Task creation (priority '3')
	xTaskCreate(ReceiverTask,"ReceiverTask",1000,NULL,3,NULL);



	vTaskStartScheduler();


	return 0;
}






#pragma GCC diagnostic pop
void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
void vApplicationIdleHook( void )
{
volatile size_t xFreeStackSpace;

	/* This function is called on each cycle of the idle task.  In this case it
	does nothing useful, other than report the amout of FreeRTOS heap that
	remains unallocated. */
	xFreeStackSpace = xPortGetFreeHeapSize();

	if( xFreeStackSpace > 100 )
	{
		/* By now, the kernel has allocated everything it is going to, so
		if there is a lot of heap remaining unallocated then
		the value of configTOTAL_HEAP_SIZE in FreeRTOSConfig.h can be
		reduced accordingly. */
	}
}
void vApplicationTickHook(void) {
}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
  /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  }
static StaticTask_t xTimerTaskTCB CCM_RAM;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH] CCM_RAM;
/* configUSE_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;
   *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
 }












