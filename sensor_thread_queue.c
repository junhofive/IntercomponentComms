/*
 * sensor_thread_queue.c
 *
 *  Created on: Feb 6, 2021
 *      Author: JO_Desktop
 */

#include "sensor_thread_queue.h"
#include "timer70.h"
#include "debug.h"

#define QUEUE_LENGTH 50
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
    if (xQueueReceive(sensor_thread_queue, &receivedMsg, portMAX_DELAY) != pdTRUE) {
        handleFatalError(SENSOR_QUEUE_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToSensorThreadQueueFromISR(SensorThreadMessage* targetMessage) {
    if (xQueueSendFromISR(sensor_thread_queue, targetMessage, NULL) != pdTRUE) {
        handleFatalError(SENSOR_QUEUE_NOT_SENT);
    }
}

