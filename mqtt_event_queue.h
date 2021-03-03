/*
 * mqtt_event_queue.h
 *
 *  Created on: Feb 28, 2021
 *      Author: JO_Desktop
 */

#ifndef MQTT_EVENT_QUEUE_H_
#define MQTT_EVENT_QUEUE_H_

#include <FreeRTOS.h>
#include <queue.h>

typedef struct mqttEventQueueMessage {
    int event;
} mqttEventQueueMessage;

void createMqttEventQueue();

mqttEventQueueMessage receiveFromMqttEventQueue();

void sendToMqttEventQueue(mqttEventQueueMessage* targetMessage);

#endif /* MQTT_EVENT_QUEUE_H_ */
