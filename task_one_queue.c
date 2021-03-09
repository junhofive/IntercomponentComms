/*
 * task_one_queue.c
 *
 *  Created on: Mar 1, 2021
 *      Author: JO_Desktop
 */

#include "task_one_queue.h"
#include <ti/drivers/dpl/HwiP.h>
#include "debug.h"

#define QUEUE_LENGTH 30

static QueueHandle_t task_one_queue = NULL;

void createTaskOneQueue() {
    task_one_queue = xQueueCreate(QUEUE_LENGTH, sizeof(taskOneQueueMessage));
   if (task_one_queue == NULL){
       handleFatalError(TASK_ONE_QUEUE_NOT_CREATED);
   }
}
taskOneQueueMessage receiveFromTaskOneQueue() {
    static taskOneQueueMessage receivedMsg;
    static BaseType_t status;
    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(task_one_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(task_one_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(TASK_ONE_QUEUE_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToTaskOneQueue(taskOneQueueMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(task_one_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(task_one_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(TASK_ONE_QUEUE_NOT_SENT);
    }
}

