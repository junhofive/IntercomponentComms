/*
 * mqtt_receive_queue.h
 *
 *  Created on: Feb 26, 2021
 *      Author: JO_Desktop
 */

#ifndef MQTT_RECEIVE_QUEUE_H_
#define MQTT_RECEIVE_QUEUE_H_

#include <FreeRTOS.h>
#include <queue.h>

#define BUFFER_SIZE 50

typedef struct mqttReceiveQueueMessage {
    int event;
    char payload[BUFFER_SIZE];
} mqttReceiveQueueMessage;

void createMqttReceiveQueue();

mqttReceiveQueueMessage receiveFromMqttReceiveQueue();

void sendToMqttReceiveQueue(mqttReceiveQueueMessage* targetMessage);

#endif /* MQTT_RECEIVE_QUEUE_H_ */
