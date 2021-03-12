/*
 * sensor_thread_queue.c
 *
 *  Created on: Feb 6, 2021
 *      Author: JO_Desktop
 */

#include "sensor_thread_queue.h"
#include <ti/drivers/dpl/HwiP.h>
#include "timer70.h"
#include "debug.h"

#define QUEUE_LENGTH 3
/* Static Variable */
static QueueHandle_t sensor_thread_queue = NULL;

void createSensorThreadQueue() {
    sensor_thread_queue = xQueueCreate(QUEUE_LENGTH, sizeof(SensorThreadMessage));
    if (sensor_thread_queue == NULL) {
        handleFatalError(SENSOR_QUEUE_NOT_CREATED);
    }
}

SensorThreadMessage receiveFromSensorThreadQueue() {
    static SensorThreadMessage receivedMsg;
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(sensor_thread_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(sensor_thread_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(SENSOR_QUEUE_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToSensorThreadQueueFromISR(SensorThreadMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(sensor_thread_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(sensor_thread_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(SENSOR_QUEUE_NOT_SENT);
    }
}

