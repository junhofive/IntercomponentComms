/*
 * mqtt_publish_queue.c
 *
 *  Created on: Feb 21, 2021
 *      Author: JO_Desktop
 */

#include <mqtt_publish_queue.h>
#include <ti/drivers/dpl/HwiP.h>
#include "debug.h"

#define QUEUE_LENGTH 50

static QueueHandle_t mqtt_publish_queue = NULL;

void createMqttPublishQueue() {
    mqtt_publish_queue = xQueueCreate(QUEUE_LENGTH, sizeof(mqttPublishQueueMessage));
   if (mqtt_publish_queue == NULL){
       handleFatalError(MQTT_PUBLISH_QUEUE_NOT_CREATED);
   }
}
mqttPublishQueueMessage receiveFromMqttPublishQueue() {
    static mqttPublishQueueMessage receivedMsg;
    static BaseType_t status;
    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(mqtt_publish_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(mqtt_publish_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_PUBLISH_QUEUE_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToMqttPublishQueue(mqttPublishQueueMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(mqtt_publish_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(mqtt_publish_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_PUBLISH_QUEUE_NOT_SENT);
    }
}
