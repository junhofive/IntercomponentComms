/*
 * statistics_queue.h
 *
 *  Created on: Mar 9, 2021
 *      Author: JO_Desktop
 */

#ifndef STATISTICS_QUEUE_H_
#define STATISTICS_QUEUE_H_

#include <FreeRTOS.h>
#include <queue.h>

typedef enum {
    TASK_ONE_STAT,
    TASK_TWO_STAT
} statType;

typedef struct statisticsQueueMessage {
    statType stat_type;
    int timer_count;
    int J_MsgCnt;
    int T_MsgCnt;
    int J_sequence;
    int T_sequence;
    int ChainCount;
} statisticsQueueMessage;

void createStatisticsQueue();

statisticsQueueMessage receiveFromStatisticsQueue();

void sendToStatisticsQueue(statisticsQueueMessage* targetMessage);

#endif /* STATISTICS_QUEUE_H_ */
