/*
 * mqtt_queue.h
 *
 *  Created on: Feb 21, 2021
 *      Author: JO_Desktop
 */

#ifndef MQTT_QUEUE_H_
#define MQTT_QUEUE_H_

#include <FreeRTOS.h>
#include <queue.h>

#define BUFFER_SIZE 50

#include <FreeRTOS.h>
#include <queue.h>

typedef struct mqttQueueMessage {
    int event;
    char payload[BUFFER_SIZE];
} mqttQueueMessage;

void createMqttQueue();

mqttQueueMessage receiveFromMqttQueue();

void sendToMqttQueue(mqttQueueMessage* targetMessage);

#endif /* MQTT_QUEUE_H_ */
