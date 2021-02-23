/*
 * mqtt_queue.c
 *
 *  Created on: Feb 21, 2021
 *      Author: JO_Desktop
 */

#include "mqtt_queue.h"
#include <ti/drivers/dpl/HwiP.h>
#include "debug.h"

#define QUEUE_LENGTH 50

static QueueHandle_t mqtt_queue = NULL;

void createMqttQueue() {
    mqtt_queue = xQueueCreate(QUEUE_LENGTH, sizeof(mqttQueueMessage));
   if (mqtt_queue == NULL){
       handleFatalError(MQTT_QUEUE_NOT_CREATED);
   }
}
mqttQueueMessage receiveFromMqttQueue() {
    static mqttQueueMessage receivedMsg;
    static BaseType_t status;
    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(mqtt_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(mqtt_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_QUEUE_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToMqttQueue(mqttQueueMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(mqtt_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(mqtt_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_QUEUE_NOT_SENT);
    }
}
