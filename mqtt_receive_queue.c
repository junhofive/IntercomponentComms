/*
 * mqtt_receive_queue.c
 *
 *  Created on: Feb 26, 2021
 *      Author: JO_Desktop
 */

#include "mqtt_receive_queue.h"
#include <ti/drivers/dpl/HwiP.h>
#include "debug.h"

#define QUEUE_LENGTH 50

static QueueHandle_t mqtt_receive_queue = NULL;

void createMqttReceiveQueue() {
    mqtt_receive_queue = xQueueCreate(QUEUE_LENGTH, sizeof(mqttReceiveQueueMessage));
   if (mqtt_receive_queue == NULL){
       handleFatalError(MQTT_RECEIVE_QUEUE_NOT_CREATED);
   }
}
mqttReceiveQueueMessage receiveFromMqttReceiveQueue() {
    static mqttReceiveQueueMessage receivedMsg;
    static BaseType_t status;
    if (HwiP_inISR()) {
        status = xQueueReceiveFromISR(mqtt_receive_queue, &receivedMsg, NULL);
    }
    else {
        status = xQueueReceive(mqtt_receive_queue, &receivedMsg, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_RECEIVE_QUEUE_NOT_RECEIVED);
    }
    return receivedMsg;
}

void sendToMqttReceiveQueue(mqttReceiveQueueMessage* targetMessage) {
    static BaseType_t status;

    if (HwiP_inISR()) {
        status = xQueueSendFromISR(mqtt_receive_queue, targetMessage, NULL);
    }
    else {
        status = xQueueSend(mqtt_receive_queue, targetMessage, portMAX_DELAY);
    }

    if (status != pdTRUE) {
        handleFatalError(MQTT_RECEIVE_QUEUE_NOT_SENT);
    }
}


