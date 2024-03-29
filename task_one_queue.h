/*
 * task_one_queue.h
 *
 *  Created on: Mar 1, 2021
 *      Author: JO_Desktop
 */

#ifndef TASK_ONE_QUEUE_H_
#define TASK_ONE_QUEUE_H_

#include <FreeRTOS.h>
#include <queue.h>

typedef enum {
    TIMER70_MSG,
    TIMER500_MSG,
    TIMER70_JASON,
    TIMER500_JASON,
    TIMER70_TERRY,
    TIMER500_TERRY
} msgType;

typedef struct taskOneQueueMessage {
    msgType message_type;
    int SensorReading;
    int SensorCount;
    int SensorAvg;
    int Time;
    int Sequence;
} taskOneQueueMessage;

void createTaskOneQueue();

taskOneQueueMessage receiveFromTaskOneQueue();

void sendToTaskOneQueue(taskOneQueueMessage* targetMessage);

#endif /* TASK_ONE_QUEUE_H_ */
