/*
 * statistics_queue.c
 *
 *  Created on: Mar 9, 2021
 *      Author: JO_Desktop
 */

#include "statistics_queue.h"
#include <ti/drivers/dpl/HwiP.h>
#include "debug.h"

#define QUEUE_LENGTH 30

static QueueHandle_t statistics_queue = NULL;

void createStatisticsQueue() {
    statistics_queue = xQueueCreate(QUEUE_LENGTH, sizeof(statisticsQueueMessage));
   if (statistics_queue == NULL){
       handleFatalError(0x77);
   }
}
statisticsQueueMessage receiveFromStatisticsQueue() {
    static statisticsQueueMessage receivedMsg;
    static BaseType_t status;
    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(statistics_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(statistics_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(0x77);
    }
    return receivedMsg;
}

void sendToStatisticsQueue(statisticsQueueMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(statistics_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(statistics_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(0x77);
    }
}

