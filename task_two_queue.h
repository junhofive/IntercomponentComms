/*
 * task_two_queue.h
 *
 *  Created on: Mar 1, 2021
 *      Author: JO_Desktop
 */

#ifndef TASK_TWO_QUEUE_H_
#define TASK_TWO_QUEUE_H_

#include <FreeRTOS.h>
#include <queue.h>

typedef enum {
    TIMER70_PAYLOAD,
    TIMER500_PAYLOAD
} payloadType;

typedef struct taskTwoQueueMessage {
    int value;
    int ChainCount;
} taskTwoQueueMessage;

void createTaskTwoQueue();

taskTwoQueueMessage receiveFromTaskTwoQueue();

void sendToTaskTwoQueue(taskTwoQueueMessage* targetMessage);

#endif /* TASK_TWO_QUEUE_H_ */
