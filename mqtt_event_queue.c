/*
 * mqtt_event_queue.c
 *
 *  Created on: Feb 28, 2021
 *      Author: JO_Desktop
 */

#include <mqtt_event_queue.h>
#include <ti/drivers/dpl/HwiP.h>
#include "debug.h"

#define QUEUE_LENGTH 5

static QueueHandle_t mqtt_event_queue = NULL;

void createMqttEventQueue() {
    mqtt_event_queue = xQueueCreate(QUEUE_LENGTH, sizeof(mqttEventQueueMessage));
   if (mqtt_event_queue == NULL){
       handleFatalError(MQTT_EVENT_QUEUE_NOT_CREATED);
   }
}
mqttEventQueueMessage receiveFromMqttEventQueue() {
    static mqttEventQueueMessage receivedMsg;
    static BaseType_t status;
    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(mqtt_event_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(mqtt_event_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_EVENT_QUEUE_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToMqttEventQueue(mqttEventQueueMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(mqtt_event_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(mqtt_event_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_EVENT_QUEUE_NOT_SENT);
    }
}

