/*Assignment 4
 * Salma Mobasher
 * 8120214*/

/*For this assignment, I attempted to do the DFS deadlock detection as follows.
 * I used 3 queues to send data to the checking task.
 * Then it puts that information into a 2dimentionsal array.
 *then it uses those arrays and also checks for some information
 *to basically keep track of what wants what and what has what in sort of a linked list way
 *then it cycles through the linked list, and if it ever comes upon the same task
 *twice, it finds itself in a loop and then drops out and declares a possible deadlock
 * */

//includes
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>
#include <FreeRTOS.h>
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes =
{
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

void SystemClock_Config(void);
void MX_GPIO_Init(void);
void StartDefaultTask(void *argument);


//writes out to the console
int write(int file, char *prt, int len)
{
    int i=0;
    taskENTER_CRITICAL();
    for(i=0; i < len; i++)
    ITM_SendChar((*prt++));
    taskEXIT_CRITICAL();
    return len;
}
void firstTask(void *pvParameters);
void secondTask(void *pvParameters);
void thirdTask(void *pvParameters);
void fourthTask(void *pvParameters);
void fifthTask(void *pvParameters); //first 5 tasks are for regular holding
void checkTask(void *pvParameters); //this task checks deadlock

xSemaphoreHandle mutex[5]; //array of 5 mutexes
TaskHandle_t xDInspector;
TaskHandle_t xTask1;
TaskHandle_t xTask2;
TaskHandle_t xTask3;
TaskHandle_t xTask4;
TaskHandle_t xTask5;
QueueHandle_t xQueueWanting; //queue for wanting mutex
QueueHandle_t xQueueAcquiring; //queue for acquiring
QueueHandle_t xQueueReleasing; //queue for releasing

struct point //it holds the element and it points toward what it wants to have
{
    int vert;
    struct point* next;
};

struct point* nodeMake(int v);

struct theData
{
    int value;
    int fromWho;
};

struct Drawing{ //puts nodes together when sorting through them
    int vertexes;
    int* seen;
    struct point** adjLists;
};

bool Method(struct Drawing* Drawing, int vert) //method for finding things
{
    struct point* inList = Drawing->adjLists[vert];
    struct point* temp = inList;

    Drawing->seen[vert] += 1;
    printf("seen %d \n", vert);

    while (temp != NULL)
    {
      int connvert = temp->vert;

      if (Drawing->seen[connvert] != 0)
      {
        printf("seen %d \n", connvert);
        printf("Found a loop! Potiential deadlock!");
        return true;
      }
      Method(Drawing, connvert);
      temp = temp->next;
    }
    return false;
}

struct point* nodeMake(int v)
{
  struct point* nodeNew = malloc(sizeof(struct point));
  nodeNew->vert = v;
  nodeNew->next = NULL;
  return nodeNew;
}

struct Drawing* createGraph(int vertexes)
{
  struct Drawing* Drawing = malloc(sizeof(struct Drawing));
  Drawing->vertexes = vertexes;

  Drawing->adjLists = malloc(vertexes * sizeof(struct point*));

  Drawing->seen = malloc(vertexes * sizeof(int));

  int i;
  for (i = 0; i < vertexes; i++)
  {
    Drawing->adjLists[i] = NULL;
    Drawing->seen[i] = 0;
  }
  return Drawing;
}

void addEdge(struct Drawing* Drawing, int src, int dest)
{
  struct point* nodeNew = nodeMake(src);
  nodeNew->next = Drawing->adjLists[dest];
  Drawing->adjLists[dest] = nodeNew;
}

void firstTask(void *pvParameters)
{
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100); //used to convert a time specified in milliseconds into a time specified in ticks.
    int x = (rand() % (2000-1000+1)) + 1000;
    int mutArray[5] = {0, 1, 2, 3, 4};
    int current[5];
    int n = 5;
    if (n > 1)
    {
        for (int i = 0; i < n -1; i++)
        {
            int j = i + rand() / (RAND_MAX / (n-i) + 1);
            int t = mutArray[j];
            mutArray[j] = mutArray[i];
            mutArray[i] = t;
        }
    }

    for (int i=0; i < 5; i++)
    {
        taskENTER_CRITICAL();
        printf("task 1 holds:"); //as requested by prof
        for (int z = 0; z < 5; z++)
            printf("[%d]", current[z]);
        printf(", wants %d \n", mutArray[i]);

        struct theData sending = {mutArray[i], 5}; //inspetion will occur on whatever has the task 1 desired mutex

        xQueueSendToBack(xQueueWanting, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        xSemaphoreTake(mutex[mutArray[i]], portMAX_DELAY);
        current[i] = mutArray[i]; //putting it in the owned mutexs array
        taskENTER_CRITICAL();

        xQueueSendToBack(xQueueAcquiring, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
    for (int i = 0; i < 5; i++)
    {
        xSemaphoreGive(mutex[mutArray[i]]);
        taskENTER_CRITICAL();
        printf("task 1 released %d", mutArray[i]);
        struct theData sending = {mutArray[i], 5};
        xQueueSendToBack(xQueueReleasing, &sending, xTicksToWait);
        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
}

void secondTask(void *pvParameters)
{
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
    int x = (rand() % (2000-1000+1)) + 1000;
    int mutArray[5] = {0, 1, 2, 3, 4};
    int current[5];
    int n = 5;
    if (n > 1)
    {
        for (int i = 0; i < n -1; i++)
        {
            int j = i + rand() / (RAND_MAX / (n-i) + 1);
            int t = mutArray[j];
            mutArray[j] = mutArray[i];
            mutArray[i] = t;
        }
    }

    for (int i=0; i < 5; i++)
    {
        taskENTER_CRITICAL();
        printf("task 2 holds:");
        for (int z = 0; z < 5; z++)
            printf("[%d]", current[z]);
        printf(", wants %d \n", mutArray[i]);

        struct theData sending = {mutArray[i], 6};
                //sending a structure with what mutex task 2 currently wants to the inspecting tasks

        xQueueSendToBack(xQueueWanting, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        xSemaphoreTake(mutex[mutArray[i]], portMAX_DELAY);
        current[i] = mutArray[i]; //putting it in the owned mutexs array
        taskENTER_CRITICAL();

        xQueueSendToBack(xQueueAcquiring, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
    for (int i = 0; i < 5; i++)
    {
        xSemaphoreGive(mutex[mutArray[i]]);
        taskENTER_CRITICAL();
        printf("task 2 released %d", mutArray[i]);
        struct theData sending = {mutArray[i], 6};
        xQueueSendToBack(xQueueReleasing, &sending, xTicksToWait);
        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
}

void thirdTask(void *pvParameters)
{
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
    int x = (rand() % (2000-1000+1)) + 1000;
    int mutArray[5] = {0, 1, 2, 3, 4};
    int current[5];
    int n = 5;
    if (n > 1)
    {
        for (int i = 0; i < n -1; i++)
        {
            int j = i + rand() / (RAND_MAX / (n-i) + 1);
            int t = mutArray[j];
            mutArray[j] = mutArray[i];
            mutArray[i] = t;
        }
    }

    for (int i=0; i < 5; i++)
    {
        taskENTER_CRITICAL();
        printf("task 3 holds:");
        for (int z = 0; z < 5; z++)
            printf("[%d]", current[z]);
        printf(", wants %d \n", mutArray[i]);

        struct theData sending = {mutArray[i], 7};

        xQueueSendToBack(xQueueWanting, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        xSemaphoreTake(mutex[mutArray[i]], portMAX_DELAY);
        current[i] = mutArray[i]; //putting it in the owned mutexs array
        taskENTER_CRITICAL();

        xQueueSendToBack(xQueueAcquiring, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
    for (int i = 0; i < 5; i++)
    {
        xSemaphoreGive(mutex[mutArray[i]]);
        taskENTER_CRITICAL();
        printf("task 3 released %d", mutArray[i]);
        struct theData sending = {mutArray[i], 7};
        xQueueSendToBack(xQueueReleasing, &sending, xTicksToWait);
        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
}

void fourthTask(void *pvParameters)
{
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
    int x = (rand() % (2000-1000+1)) + 1000; //used for delay
    int mutArray[5] = {0, 1, 2, 3, 4};
    int current[5];
    int n = 5;
    if (n > 1)
    {
        for (int i = 0; i < n -1; i++)
        {
            int j = i + rand() / (RAND_MAX / (n-i) + 1);
            int t = mutArray[j];
            mutArray[j] = mutArray[i];
            mutArray[i] = t;
        }
    }

    for (int i=0; i < 5; i++)
    {
        taskENTER_CRITICAL();
        printf("task 4 holds:");
        for (int z = 0; z < 5; z++)
            printf("[%d]", current[z]);
        printf(", wants %d \n", mutArray[i]);

        struct theData sending = {mutArray[i], 8};

        xQueueSendToBack(xQueueWanting, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        xSemaphoreTake(mutex[mutArray[i]], portMAX_DELAY);
        current[i] = mutArray[i]; //putting it in the owned mutexs array
        taskENTER_CRITICAL();

        xQueueSendToBack(xQueueAcquiring, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
    for (int i = 0; i < 5; i++)
    {
        xSemaphoreGive(mutex[mutArray[i]]);
        taskENTER_CRITICAL();
        printf("task 4 released %d", mutArray[i]);
        struct theData sending = {mutArray[i], 8};
        xQueueSendToBack(xQueueReleasing, &sending, xTicksToWait);
        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
}

void fifthTask(void *pvParameters)
{
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);
    int x = (rand() % (2000-1000+1)) + 1000;
    int mutArray[5] = {0, 1, 2, 3, 4};
    int current[5];
    int n = 5;
    if (n > 1)
    {
        for (int i = 0; i < n -1; i++)
        {
            int j = i + rand() / (RAND_MAX / (n-i) + 1);
            int t = mutArray[j];
            mutArray[j] = mutArray[i];
            mutArray[i] = t;
        }
    }

    for (int i=0; i < 5; i++)
    {
        taskENTER_CRITICAL();
        printf("task 5 holds:");
        for (int z = 0; z < 5; z++)
            printf("[%d]", current[z]);
        printf(", wants %d \n", mutArray[i]);

        struct theData sending = {mutArray[i], 9};

        xQueueSendToBack(xQueueWanting, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        xSemaphoreTake(mutex[mutArray[i]], portMAX_DELAY);
        current[i] = mutArray[i];
        taskENTER_CRITICAL();

        xQueueSendToBack(xQueueAcquiring, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }

    for (int i = 0; i < 5; i++)
    {
        xSemaphoreGive(mutex[mutArray[i]]);
        taskENTER_CRITICAL();
        printf("task 5 released %d", mutArray[i]);

        struct theData sending = {mutArray[i], 9};
        xQueueSendToBack(xQueueReleasing, &sending, xTicksToWait);

        taskEXIT_CRITICAL();
        vTaskDelay(x);
    }
}

void checkTask(void *pvParemters)
{

	int queueOfWants[5]; //this states what each thing wants
	int queueGotten[5][5]; //this states what each thing already has
	int a=0, b=0, c=0, d=0, e = 0; //semaphors

	bool deadlock = false;
	int vertexes = 0;
	struct theData TempPlace;
	BaseType_t xStatus;
	struct Drawing* Drawing = createGraph(10);
	for(int i = 0; i < uxQueueMessagesWaiting(xQueueWanting); i++)
	{
		xStatus = xQueueReceive(xQueueWanting, &TempPlace, 0);//indicates the desired things
		if(xStatus == pdPASS)
		{
			if (TempPlace.fromWho == 5)
			{
				queueOfWants[0] = TempPlace.value;
			}
			else if(TempPlace.fromWho == 6)
			{
				queueOfWants[1] = TempPlace.value;
			}
			else if(TempPlace.fromWho == 7)
			{
				queueOfWants[2] = TempPlace.value;
			}
			else if(TempPlace.fromWho == 8)
			{
				queueOfWants[3] = TempPlace.value;
			}
			else if(TempPlace.fromWho == 9)
			{
				queueOfWants[4] = TempPlace.value;
			}
		}
	}
	for (int i = 0; i < uxQueueMessagesWaiting(xQueueAcquiring); i++)
	{
		xStatus = xQueueReceive(xQueueAcquiring, &TempPlace, 0); //puts semaphors where they belong
		if(xStatus == pdPASS)
		{
			if (TempPlace.fromWho == 5)
			{
				queueGotten[0][a] = TempPlace.value;
				a++;
				struct point* nodeNew = nodeMake(TempPlace.value);
				nodeNew->next = TempPlace.fromWho; //creates a point with resource it wants
				Drawing->adjLists[TempPlace.fromWho] = nodeNew;
				vertexes++;

			}
			else if(TempPlace.fromWho == 6)
			{
				queueGotten[1][b] = TempPlace.value;
				b++;
				queueGotten[0][a] = TempPlace.value;
				a++;
				struct point* nodeNew = nodeMake(TempPlace.value);
				nodeNew->next = TempPlace.fromWho;
				Drawing->adjLists[TempPlace.fromWho] = nodeNew;
				vertexes++;

			}
			else if(TempPlace.fromWho == 7)
			{
				queueGotten[2][c] = TempPlace.value;
				c++;
				queueGotten[0][a] = TempPlace.value;
				a++;
				struct point* nodeNew = nodeMake(TempPlace.value);
				nodeNew->next = TempPlace.fromWho;
				Drawing->adjLists[TempPlace.fromWho] = nodeNew;
				vertexes++;
			}
			else if(TempPlace.fromWho == 8)
			{
				queueGotten[3][d] = TempPlace.value;
				d++;
				queueGotten[0][a] = TempPlace.value;
				a++;
				struct point* nodeNew = nodeMake(TempPlace.value);
				nodeNew->next = TempPlace.fromWho;
				Drawing->adjLists[TempPlace.fromWho] = nodeNew;
				vertexes++;
			}
			else if(TempPlace.fromWho == 9)
			{
				queueGotten[4][e] = TempPlace.value;
				e++;
				queueGotten[0][a] = TempPlace.value;
				a++;
				struct point* nodeNew = nodeMake(TempPlace.value);
				nodeNew->next = TempPlace.fromWho;
				Drawing->adjLists[TempPlace.fromWho] = nodeNew;
				vertexes++;
			}
		}
	}
	for (int i = 0; i < uxQueueMessagesWaiting(xQueueReleasing); i++)
	{
		xStatus = xQueueReceive(xQueueReleasing, &TempPlace, 0);//takes out sempahore that are already done with
		if(xStatus == pdPASS)
		{
			if (TempPlace.fromWho == 5)
			{
				for (int x = 5; x < 5; x++)
				{
					if(queueGotten[0][x] == TempPlace.value)
					{
						queueGotten[0][x] = NULL;
						a--;
					}
				}
			}
			else if(TempPlace.fromWho == 6)
			{
				for (int x = 0; x < 5; x++)
				{
					if(queueGotten[1][x] == TempPlace.value)
					{
						queueGotten[1][x] = NULL;
						b--;
					}
				}
			}
			else if(TempPlace.fromWho == 7)
			{
				for (int x = 0; x < 5; x++)
				{
					if(queueGotten[2][x] == TempPlace.value)
					{
						queueGotten[2][x] = NULL;
						c--;
					}
				}
			}
			else if(TempPlace.fromWho == 8)
			{
				for (int x = 0; x < 5; x++)
				{
					if(queueGotten[3][x] == TempPlace.value)
					{
						queueGotten[3][x] = NULL;
						d--;
					}
				}
			}
			else if(TempPlace.fromWho == 9)
			{
				for (int x = 0; x < 5; x++)
				{
					if(queueGotten[4][x] == TempPlace.value)
					{
						queueGotten[4][x] = NULL;
						e--;//sempahores are removed in backwards order!
					}
				}
			}
		}
	}
	for(int i = 0; i < 5; i++)
	{
		if (queueGotten[0][i] == queueOfWants[0])
		{
			struct point* nodeNew = nodeMake(TempPlace.fromWho);
			nodeNew->next = TempPlace.value;//creates node while not having what it wants
			Drawing->adjLists[TempPlace.value] = nodeNew;
		}
		if (queueGotten[1][i] == queueOfWants[0])
		{
			struct point* nodeNew = nodeMake(TempPlace.fromWho);
			nodeNew->next = TempPlace.value;
			Drawing->adjLists[TempPlace.value] = nodeNew;
		}
		if (queueGotten[2][i] == queueOfWants[0])
		{
			struct point* nodeNew = nodeMake(TempPlace.fromWho);
			nodeNew->next = TempPlace.value;.
			Drawing->adjLists[TempPlace.value] = nodeNew;
		}
		if (queueGotten[3][i] == queueOfWants[0])
		{
			struct point* nodeNew = nodeMake(TempPlace.fromWho);
			nodeNew->next = TempPlace.value;
			Drawing->adjLists[TempPlace.value] = nodeNew;
		}
		if (queueGotten[4][i] == queueOfWants[0])
		{
			struct point* nodeNew = nodeMake(TempPlace.fromWho);
			nodeNew->next = TempPlace.value;
			Drawing->adjLists[TempPlace.value] = nodeNew;
		}
	}

	//checking for deadlock
	for (int i = 0; i < 10; i ++)
		deadlock = Method(Drawing, i);

	if(deadlock == true)
		{	//searching
			vTaskSuspend(xTask1);
			vTaskSuspend(xTask2);
			vTaskSuspend(xTask3);
			vTaskSuspend(xTask4);
			vTaskSuspend(xTask5);
			taskENTER_CRITICAL();
			printf("Inspection is on! Deadlock found!!!\n");
			taskEXIT_CRITICAL();
			while(1)
			{

			}
		}
	else
	{
		taskENTER_CRITICAL();
		printf("Inspection is on! No deadlock here!!!\n");
		taskEXIT_CRITICAL();
	}
	vTaskDelay(500);
}


int main(void)
{
    xQueueWanting = xQueueCreate(25, sizeof(int) ); //creating queues
    xQueueAcquiring = xQueueCreate(25, sizeof(int) );
    xQueueReleasing = xQueueCreate(25, sizeof(int) );
    xTaskCreate(firstTask, "Task 1", 1000, NULL, 1, NULL); //calline functions
    xTaskCreate(secondTask, "Task 2", 1000, NULL, 1, NULL);
    xTaskCreate(thirdTask, "Task 3", 1000, NULL, 1, NULL);
    xTaskCreate(fourthTask, "Task 4", 1000, NULL, 1, NULL);
    xTaskCreate(fifthTask, "Task 5", 1000, NULL, 1, NULL);
    xTaskCreate(checkTask, "Check Task", 1000, NULL, 2, &xDInspector);
    for (int i = 0; i < 5; i++)
    {
        mutex[i] = xSemaphoreCreateMutex(); //creating mutexes
    }
    vTaskStartScheduler(); //staaart
    //os stuff
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    osKernelInitialize();
    defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
    osKernelStart();
    while(1)
    {
    }
}
