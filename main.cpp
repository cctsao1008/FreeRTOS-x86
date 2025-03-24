#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stream_buffer.h"
#include "event_groups.h"

// examples for stream buffer
#define BUFFER_SIZE 256
#define TRIGGER_LEVEL 1

StreamBufferHandle_t xStreamBuffer;  // Stream buffer handle

// Simulated UART ISR function (runs in a separate thread)
DWORD WINAPI vUARTISR(LPVOID lpParam) {
    char receivedChar;
    
    while (1) {
        receivedChar = 'A' + (rand() % 26);  // Simulated incoming UART data
        xStreamBufferSend(xStreamBuffer, &receivedChar, 1, portMAX_DELAY);
        Sleep(100);  // Simulate delay between received bytes
    }

    return 0;
}

// Task to process received UART data
void vUARTReceiveTask(void *pvParameters) {
    char receivedData[BUFFER_SIZE];
    size_t bytesRead;

    while (1) {
        bytesRead = xStreamBufferReceive(xStreamBuffer, receivedData, sizeof(receivedData), pdMS_TO_TICKS(500));

        if (bytesRead > 0) {
            receivedData[bytesRead] = '\0';  // Null-terminate for safe printing
            printf("%s Received: %s\n", pcTaskGetName(NULL), receivedData);
        }

        vTaskDelay(pdMS_TO_TICKS(4000));
    }
}

// examples for xQueueSend
xQueueHandle queue;

void TxTask(void *pvParameters) {
    float x = 0;
    while (true) {
        printf("%s sending %f...\n", pcTaskGetName(NULL), x);
        xQueueSend(queue, &x, portMAX_DELAY);
        x += 0.1;
        vTaskDelay(1000); // 1 second
    }
}

void RxTask(void *pvParameters) {
    while (true) {
        float received;
        if (xQueueReceive(queue, &received, portMAX_DELAY) == pdTRUE) {
            printf("%s received: %f\n", pcTaskGetName(NULL), received);
        }
    }
}

// examples for task
void Task1(void *pvParameters) {
    while(1)
    {
        printf("hello %s ...\n", pcTaskGetName(NULL));
        vTaskDelay(1000); // 1 second
    }
}

void Task2(void *pvParameters) {
    while(1)
    {
        printf("hello %s ...\n", pcTaskGetName(NULL));
        vTaskDelay(1000); // 1 second
    }
}

// examples for using pvParameters in the task
typedef struct {
    int taskID;
    char taskName[64];
} TaskParams_t;

void TaskFunction(void *pvParameters) {
    TaskParams_t *params = (TaskParams_t *)pvParameters;  // Cast to struct pointer

    while (1) {
        printf("%s is running, ID = %d\n", params->taskName, params->taskID);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

// examples for using event group
// Define event bits
#define EVENT_BIT_0  (1 << 0)  // 0x01
#define EVENT_BIT_1  (1 << 1)  // 0x02

EventGroupHandle_t xEventGroup; // Handle for the event group

void vTask6(void *pvParameters) {
    while (1) {
        printf("Task 6 setting EVENT_BIT_0\n");
        xEventGroupSetBits(xEventGroup, EVENT_BIT_0); // Set bit 0
        vTaskDelay(pdMS_TO_TICKS(1000)); // Simulate work
    }
}

void vTask7(void *pvParameters) {
    while (1) {
        printf("Task 7 setting EVENT_BIT_1\n");
        xEventGroupSetBits(xEventGroup, EVENT_BIT_1); // Set bit 1
        vTaskDelay(pdMS_TO_TICKS(1500)); // Simulate work
    }
}

void vTaskWait(void *pvParameters) {
    while (1) {
        // Wait until both EVENT_BIT_0 and EVENT_BIT_1 are set
        EventBits_t uxBits = xEventGroupWaitBits(
            xEventGroup,      // Event group handle
            EVENT_BIT_0 | EVENT_BIT_1, // Bits to wait for
            pdTRUE,           // Clear bits on exit
            pdTRUE,           // Wait for all bits
            portMAX_DELAY     // Wait forever
        );

        if ((uxBits & (EVENT_BIT_0 | EVENT_BIT_1)) == (EVENT_BIT_0 | EVENT_BIT_1)) {
            printf("TaskWait: Both events received!\n");
            printf("TaskWait clear EVENT_BIT_0 and EVENT_BIT_1\n");
            xEventGroupClearBits(xEventGroup, EVENT_BIT_0 | EVENT_BIT_1); // Clear bit 0 and bit 1
        }
        
        vTaskDelay(pdMS_TO_TICKS(1500)); // Simulate work
    }
}

int main() {
    // Create the event group
    queue = xQueueCreate(1000, sizeof(float));
    
    if( queue == NULL )
    {
        /* Queue was not created and must not be used. */
        printf("queue was not created.");
        return (EXIT_FAILURE);
    }
    
    // Create the event group
    xEventGroup = xEventGroupCreate();

    if (xEventGroup == NULL) {
        printf("Failed to create event group\n");
        return (EXIT_FAILURE);
    }

    xTaskCreate(RxTask, "RxTask", 256, NULL, 1, NULL);
    xTaskCreate(TxTask, "TxTask", 256, NULL, 1, NULL);
    
    xTaskCreate(Task1, "Task1", 256, NULL, 1, NULL);
    xTaskCreate(Task2, "Task2", 256, NULL, 1, NULL);
    
    static TaskParams_t task1Params = {3, "Task3"};
    static TaskParams_t task2Params = {4, "Task4"};

    xTaskCreate(TaskFunction, "Task3", 256, &task1Params, 1, NULL);
    xTaskCreate(TaskFunction, "Task4", 256, &task2Params, 1, NULL);

    // Create a stream buffer
    xStreamBuffer = xStreamBufferCreate(BUFFER_SIZE, TRIGGER_LEVEL);
    if (xStreamBuffer == NULL) {
        printf("Failed to create stream buffer!\n");
        return 1;
    }

    // Create a FreeRTOS task for processing UART data
    xTaskCreate(vUARTReceiveTask, "Task5", 256, NULL, 2, NULL);

    // Create tasks
    xTaskCreate(vTask6, "Task6", 256, NULL, 2, NULL);
    xTaskCreate(vTask7, "Task7", 256, NULL, 2, NULL);
    xTaskCreate(vTaskWait, "TaskWait", 1024, NULL, 1, NULL);

    // Simulate an ISR using a separate Windows thread(Windows-only)
    HANDLE hThread = CreateThread(NULL, 0, vUARTISR, NULL, 0, NULL);
    if (hThread == NULL) {
        printf("Failed to create ISR thread!\n");
        return 1;
    }

    vTaskStartScheduler();
    return 0;
}

