/*
 * mqtt_queue.h
 *
 *  Created on: Feb 21, 2021
 *      Author: JO_Desktop
 */

#ifndef MQTT_PUBLISH_QUEUE_H_
#define MQTT_PUBLISH_QUEUE_H_

#include <FreeRTOS.h>
#include <queue.h>

#define BUFFER_SIZE 50

typedef struct mqttPublishQueueMessage {
    int event;
    char payload[BUFFER_SIZE];
} mqttPublishQueueMessage;

void createMqttPublishQueue();

mqttPublishQueueMessage receiveFromMqttPublishQueue();

void sendToMqttPublishQueue(mqttPublishQueueMessage* targetMessage);

#endif /* MQTT_PUBLISH_QUEUE_H_ */
