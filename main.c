/*----------------------------------------------------------------------------
	Circular Buffer Implementation
*----------------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "cmsis_os.h"
#include "uart.h"

void Producer_Thread (void const *argument);
void Consumer_Thread (void const *argument);
osThreadDef(Producer_Thread, osPriorityNormal, 1, 0);
osThreadDef(Consumer_Thread, osPriorityNormal, 1, 0);

osThreadId T_x1; // Thread ID
osThreadId T_x2; // Thread ID
	
osMutexId x_mutex; // Mutex ID
osMutexDef(x_mutex); // Mutex definition
osSemaphoreId item_semaphore; // Semaphore ID
osSemaphoreDef(item_semaphore); // Semaphore definition
osSemaphoreId space_semaphore; // Semaphore ID
osSemaphoreDef(space_semaphore); // Semaphore definition

long int x=0;
long int i=0;
long int j=0;

// Structure of the circular buffer (contain 8 spaces)
const unsigned int N = 7; 
unsigned char buffer[N]; 
unsigned int insertPtr = 0; //increase insertPtr when item is put into buffer
unsigned int removePtr = 0; // increase removedPtr when item is get from buffer

/*----------------------------------------------------------------------------
Producer insert item to circular buffer
*----------------------------------------------------------------------------*/
void put(unsigned char an_item){
	unsigned int P = 0x50;
	unsigned int u = 0x75;
	unsigned int t = 0x74;
	unsigned int sym = 0x3A;
	osSemaphoreWait(space_semaphore, osWaitForever); // wait for empty space
	osMutexWait(x_mutex, osWaitForever); // guarantee the atomic (protect critical section)
	buffer[insertPtr] = an_item; // insert item
	insertPtr = (insertPtr + 1) % N; // %N is used mainly to provide circular structure (7->0) 
	SendChar(P);
	SendChar(u);
	SendChar(t);
	SendChar(sym);
	SendChar(an_item);
	SendChar('\n');
	osMutexRelease(x_mutex);
	osSemaphoreRelease(item_semaphore);
}

/*----------------------------------------------------------------------------
Consumer get item from circular buffer
*----------------------------------------------------------------------------*/
unsigned char get(){
	unsigned int item = 0x00; // initialise variable for storing the item get from buffer
	unsigned int G = 0x47;
	unsigned int e = 0x65;
	unsigned int t = 0x74;
	unsigned int sym = 0x3A;
	osSemaphoreWait(item_semaphore, osWaitForever); // wait for item
	osMutexWait(x_mutex, osWaitForever); // guarantee the atomic (protect critical section)
	item = buffer[removePtr]; // get item and store to variable "item"
	removePtr = (removePtr + 1) % N; // %N is used mainly to provide circular structure (7->0) 
	SendChar(G);
	SendChar(e);
	SendChar(t);
	SendChar(sym);
	osMutexRelease(x_mutex);
	osSemaphoreRelease(space_semaphore);
	return item;
}

int loopcount = 26;

/*----------------------------------------------------------------------------
Producer Thread (item insert from A-Z)
*----------------------------------------------------------------------------*/
void Producer_Thread (void const *argument) 
{
	unsigned char item = 0x41;
	for(; i%3; i++){
		put(item++);
	}
}
/*----------------------------------------------------------------------------
Consumer Thread (get item from A-Z)
*----------------------------------------------------------------------------*/
void Consumer_Thread (void const *argument) 
{
	for(; j%5; j++){
		unsigned int data = 0x00;
		data = get();
		SendChar(data);
		SendChar('\n');
	}
}

int main (void) 
{
	osKernelInitialize ();                    // initialize CMSIS-RTOS
	USART1_Init();
	item_semaphore = osSemaphoreCreate(osSemaphore(item_semaphore), 0);
	space_semaphore = osSemaphoreCreate(osSemaphore(space_semaphore), N); 
	x_mutex = osMutexCreate(osMutex(x_mutex));	
	
	T_x1 = osThreadCreate(osThread(Producer_Thread), NULL); // producer
	T_x2 = osThreadCreate(osThread(Consumer_Thread), NULL); // consumer
 
	osKernelStart ();                         // start thread execution 
}


