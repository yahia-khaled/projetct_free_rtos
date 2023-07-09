#include <stdio.h>
#include <stdlib.h>
#include "diag/trace.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "semphr.h"
#include "time.h"

#define CCM_RAM __attribute__((section(".ccmram")))
#define QUEUE_SIZE 3


// initializing global variables
int LowerBoundarr [] = {50, 80, 110, 140, 170, 200};
int HigherBoundarr []= {150, 200, 250, 300, 350, 400};
int index=-1; 				//index for the two above arrays to determine when the two arrays end
uint32_t seed ; // using in Generate random period
BaseType_t StartPeriod=1;
BaseType_t BeginProgram =1;

int sender_period[3]; 	//store period of Sender1 , Sender2 , Sender3
int SenderCounter[3] ={0};		//counter to calculate the number of sent messages from Sender1 ,Sender2 , Sender3.
int receiver_period;	//store period of the receiver
int received_Counter_Messages =0;	//counter to calculate the number of received messages at the receiver

int blocked_msg[3] ={0};	//counter to calculate the number of blocked messages from Sender1 ,Sender2 , Sender3.

//Global Tasks, Queue, Timers and Semaphores handle
TaskHandle_t HandelSender1 =0;
TaskHandle_t HandelSender2 =0;
TaskHandle_t HandelSender3 =0;
TaskHandle_t HandelReceiver=0;

QueueHandle_t MessageQueue =0;

TimerHandle_t TimerSender_1 =0;
TimerHandle_t TimerSender_2 =0;
TimerHandle_t TimerSender_3 =0;
TimerHandle_t TimerReceiver=0;

SemaphoreHandle_t SemaphoreTX1;
SemaphoreHandle_t SemaphoreTX2;
SemaphoreHandle_t SemaphoreTX3;
SemaphoreHandle_t SemaphoreRX;



//Sender Tasks function
void SenderTasks(int parameter_send)
{
int TaskID =parameter_send;
	if (TaskID==0)//You are in Task 1
	{
	char MySender_Message [30];
	BaseType_t txstatus1;
	while(1)
	{
		xSemaphoreTake(SemaphoreTX1, portMAX_DELAY);
		int timer= xTaskGetTickCount();
		//adding the integer timer to the string in one variable MySender's_Message
		snprintf(MySender_Message, 30, "Time is %d", timer);
		txstatus1 = xQueueSend( MessageQueue , &MySender_Message, 0 );
		if ( txstatus1 != pdPASS )
		{
			blocked_msg[0]++;
		}
		else {
			SenderCounter[0]++;
		     }
		}
	}
	if (TaskID==1) // You are in Task 2
	{
	char MySender_Message [30];
	BaseType_t txstatus2;

	while(1)
	{
		xSemaphoreTake(SemaphoreTX2, portMAX_DELAY);
		int timer= xTaskGetTickCount();
		//adding the integer timer to the string in one variable MySender's_Message
		snprintf(MySender_Message, 30, "Time is %d", timer);
		txstatus2 = xQueueSend( MessageQueue , &MySender_Message, 0 );
		if ( txstatus2 != pdPASS )
		{
			blocked_msg[1]++;
		}
		else
			{
			SenderCounter[1]++;
			}
	}
	}
	if (TaskID==2) // You are in Task 3
	{
	char MySender_Message [30];
	BaseType_t txstatus3;

	while(1)
	{
		xSemaphoreTake(SemaphoreTX3, portMAX_DELAY);
		int timer= xTaskGetTickCount();
		//adding the integer timer to the string in one variable MySender's_Message
		snprintf(MySender_Message, 30, "Time is %d", timer);
		txstatus3 = xQueueSend( MessageQueue , &MySender_Message, 0 );
		if ( txstatus3 != pdPASS )
		{
			blocked_msg[2]++;
		}
		else {
			SenderCounter[2]++;
		}
	}
	}

}

//Receiver Task function
void ReceiverTask()
{
	char MyReceiver_Message[30];// Buffer to store received messages
	BaseType_t rxstatus;// Receive status variable

	while(1)
	{
		xSemaphoreTake(SemaphoreRX, portMAX_DELAY);// Wait for the semaphore signal
		// Receive a message from the message queue with zero block time
		rxstatus = xQueueReceive( MessageQueue , &MyReceiver_Message, 0 );

		if( rxstatus == pdPASS )
		{
			received_Counter_Messages++;//sent successfully ,  Increment the count of successfully received messages
			if(received_Counter_Messages%100 == 0 &&received_Counter_Messages!=1000)
			printf("Receiving Messages are %d , Waiting....\n",received_Counter_Messages);
           if(BeginProgram==1)
           {
        	   printf("Running\n");
        	   printf("iteration 1 From [50] to [150]\n");
        	   BeginProgram=0;
           }
			if (received_Counter_Messages == 1000)
				{
		Reset_Function();// Reset function called after receiving 1000 messages
				}
		}

	}

}

//Callback Function of the first timer that starts sender1 task
void Tsender1TimerCallback( TimerHandle_t TimerSender_1 )
{
	xSemaphoreGive(SemaphoreTX1);
	BaseType_t XChange;
	sender_period[0] = UniformlyDistributedTimer (LowerBoundarr[index] , HigherBoundarr[index]);
	//change the timer period
	XChange = xTimerChangePeriod(TimerSender_1, pdMS_TO_TICKS(sender_period[0]), 0);
	//if(XChange==pdPASS)print("True Change \n");

}

//Callback Function of the second timer that starts sender2 task
void Tsender2TimerCallback( TimerHandle_t TimerSender_2 )
{
	xSemaphoreGive(SemaphoreTX2);
	BaseType_t XChange;
	sender_period[1] = UniformlyDistributedTimer (LowerBoundarr[index] , HigherBoundarr[index]);
	XChange = xTimerChangePeriod(TimerSender_2, pdMS_TO_TICKS(sender_period[1]), 0);
//if(XChange==pdPASS)print("True Change \n");
}
//Callback Function of the second timer that starts sender3 task
void Tsender3TimerCallback( TimerHandle_t TimerSender_3 )
{
	xSemaphoreGive(SemaphoreTX3);
	BaseType_t XChange;

	sender_period[2] = UniformlyDistributedTimer (LowerBoundarr[index] , HigherBoundarr[index]);
	XChange = xTimerChangePeriod(TimerSender_3, pdMS_TO_TICKS(sender_period[2]), 0);
	//if(XChange==pdPASS)print("True Change \n");
}

//Callback Function of the third timer that starts receiver task
void TreceiverTimerCallback( TimerHandle_t TimerReceiver )
{
	xSemaphoreGive(SemaphoreRX);

}

// Random number generation function between two specific values
int UniformlyDistributedTimer (int Low , int High )
{
// Check if it's the first period and initialize the random number generator's seed
	if(StartPeriod==1)
	{
 // Set the seed value based on current time
		srand(time(NULL));
		seed=rand();
		StartPeriod=0;
	}
// XOR-shift operations to update the seed value
	    seed ^= seed << 13;
	    seed ^= seed >> 17;
	    seed ^= seed << 5;
// Calculate the range of random numbers
	int range = High - Low + 1;
// Generate a scaled random number within the range
	int myRand_scaled = (seed % range) +Low;
// Increment the seed for the next random number generation
	seed++;
// Return the generated random number
	return myRand_scaled;
}

//function to clear queues, set sender1, sender2 ,sender3  and receiver periodic time and end the scheduler after 6 iterations
//function to clear queues, set sender1, sender2 ,sender3  and receiver periodic time and end the scheduler after 6 iterations
void Intializing_Function ()
{

	xQueueReset( MessageQueue  );
	index ++;
	if(index!=6 && index!=0)
	{
		printf(" Iteration %d From [%d] to [%d] \n", index+1 ,LowerBoundarr[index],HigherBoundarr[index]);
	}
	if (index == 6)
						{
		//Delete the Timers for Sender and receiver
							xTimerDelete(TimerSender_1,0);
							xTimerDelete(TimerSender_2,0);
							xTimerDelete(TimerSender_3,0);
							xTimerDelete(TimerReceiver,0);
							printf("Game Over\n");
							exit(0);
						}

	sender_period[0] = UniformlyDistributedTimer (LowerBoundarr[index] , HigherBoundarr[index]);
	sender_period[1] = UniformlyDistributedTimer (LowerBoundarr[index] , HigherBoundarr[index]);
	sender_period[2] = UniformlyDistributedTimer (LowerBoundarr[index] , HigherBoundarr[index]);
	receiver_period = 100;


}
//reset function to print the final output of each iteration after 500 message received and clear any stored values to start a new one
void Reset_Function()
{
	//calculation for printing
	int Totalsent = SenderCounter[0] + SenderCounter[1] + SenderCounter[2];
	int TotalBlocked= blocked_msg[0]+ blocked_msg[1] + blocked_msg[2];
	//printing Our data
	printf("Receiving 1000 message was ended in Iteration %d \n",index+1);
	printf("Total Successfully sent messages are  %d  \r\n" , Totalsent );
	printf("Total Blocked messages are %d \r\n" , TotalBlocked );
	printf("For Task 1 :\n");
	printf("Successful send messages are %d  and Blocked messages are %d \n",SenderCounter[0] ,blocked_msg[0] );
		printf("For Task 2 :\n");
	printf("Successful send messages are %d  and Blocked messages are %d \n",SenderCounter[1] ,blocked_msg[1] );
		printf("For Task 3 :\n");
	printf("Successful send messages are %d  and Blocked messages are %d \n",SenderCounter[2] ,blocked_msg[2] );
	//Initial all counters For Senders
	SenderCounter[0] = 0;
	SenderCounter[1] = 0;
	SenderCounter[2] = 0;
	blocked_msg[0] = 0;
	blocked_msg[1] = 0;
	blocked_msg[2] = 0;
	//Initial Receiver counter
	received_Counter_Messages = 0;
	Intializing_Function();

}



// ----------------------------------------------------------------------------
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wreturn-type"
// ----------------------------------------------------------------------------



void main()
{
	//creating queue
	char mydata[30];
	MessageQueue  = xQueueCreate( QUEUE_SIZE , sizeof( mydata ) );
	//initialize the program
		Intializing_Function ();
	//creating semaphores
		SemaphoreTX1 = xSemaphoreCreateBinary();
		SemaphoreTX2 = xSemaphoreCreateBinary();
		SemaphoreTX3 = xSemaphoreCreateBinary();
		SemaphoreRX = xSemaphoreCreateBinary();
	//create timers
	TimerSender_1 = xTimerCreate( "Tsender1", pdMS_TO_TICKS(sender_period[0]), pdTRUE, ( void * ) 0, Tsender1TimerCallback);
	TimerSender_2 = xTimerCreate( "Tsender2", pdMS_TO_TICKS(sender_period[1]), pdTRUE, ( void * ) 0, Tsender2TimerCallback);
	TimerSender_3 = xTimerCreate( "Tsender3", pdMS_TO_TICKS(sender_period[2]), pdTRUE,   ( void * ) 0  , Tsender3TimerCallback);
	TimerReceiver = xTimerCreate( "Treceiver", pdMS_TO_TICKS(receiver_period), pdTRUE, ( void * ) 0, TreceiverTimerCallback);

		BaseType_t StatutsTaskSender_1;
		BaseType_t StatutsTaskSender_2;
		BaseType_t StatutsTaskSender_3;
		BaseType_t StatutsTaskReciver;

	if( MessageQueue  != NULL )
	{
		//create tasks
		StatutsTaskSender_1 = xTaskCreate( SenderTasks , "Sender1" , 1000,( void * ) 0, 1 , &HandelSender1  );
		StatutsTaskSender_2 = xTaskCreate( SenderTasks , "Sender2" , 1000,( void * ) 1, 1 , &HandelSender2  );
		StatutsTaskSender_3 = xTaskCreate( SenderTasks , "Sender3" , 1000,( void * ) 2, 2 , &HandelSender3  );
		StatutsTaskReciver  = xTaskCreate( ReceiverTask,"Receiver" , 1000, NULL       , 3 , &HandelReceiver );

	if( StatutsTaskSender_1 == pdPASS && StatutsTaskSender_2 == pdPASS && StatutsTaskSender_3 == pdPASS && StatutsTaskReciver == pdPASS)
	   {
		//start the scheduler
		printf("Successful creation Tasks  \r\n");

		}
	}
	else printf("Queue could not be created. \r\n");

	BaseType_t timerStarted1;
	BaseType_t timerStarted2;
	BaseType_t timerStarted3;
	BaseType_t timerStarted4;

	if( TimerSender_1!= NULL && TimerSender_2 != NULL && TimerSender_3 != NULL && TimerReceiver != NULL)
	{
		timerStarted1 = xTimerStart(TimerSender_1, 0 );
		timerStarted2 = xTimerStart(TimerSender_2, 0 );
		timerStarted3 = xTimerStart(TimerSender_3, 0 );
		timerStarted4 = xTimerStart(TimerReceiver, 0 );
	}

	if( timerStarted1 == pdPASS && timerStarted2 == pdPASS && timerStarted3 == pdPASS && timerStarted4 == pdPASS )
	{
		//start the scheduler
		printf("Start Scheduler. \r\n");
		printf("Hint:: Code start Running if Message 'Running' is appear\n");
		vTaskStartScheduler();
	}

	exit(0); // Terminate the program
}



// ----------------------------------------------------------------------------
#pragma GCC diagnostic pop
// ----------------------------------------------------------------------------

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	for( ;; );
}
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
	( void ) pcTaskName;
	( void ) pxTask;

	/* Run time stack overflow checking is performed if
	configconfigCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
	function is called if a stack overflow is detected. */
	for( ;; );
}
/*-----------------------------------------------------------*/

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

void vApplicationTickHook(void) {}

StaticTask_t xIdleTaskTCB CCM_RAM;
StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] CCM_RAM;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize)
{
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

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

