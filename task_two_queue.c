/*
 * task_two_queue.c
 *
 *  Created on: Mar 1, 2021
 *      Author: JO_Desktop
 */

#include "task_two_queue.h"
#include <ti/drivers/dpl/HwiP.h>
#include "debug.h"

#define QUEUE_LENGTH 50

static QueueHandle_t task_two_queue = NULL;

void createTaskTwoQueue() {
    task_two_queue = xQueueCreate(QUEUE_LENGTH, sizeof(taskTwoQueueMessage));
   if (task_two_queue == NULL){
//       handleFatalError(task_two_queue_NOT_CREATED);
   }
}
taskTwoQueueMessage receiveFromTaskTwoQueue() {
    static taskTwoQueueMessage receivedMsg;
    static BaseType_t status;
    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(task_two_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(task_two_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
//        handleFatalError(task_two_queue_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToTaskTwoQueue(taskTwoQueueMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(task_two_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(task_two_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
//        handleFatalError(task_two_queue_NOT_SENT);
    }
}

