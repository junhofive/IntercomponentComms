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

#define BUFFER_SIZE 120

typedef enum {
    TASK_ONE_TOPIC,
    TASK_TWO_TOPIC,
    STATUS_TOPIC
} topicType;

typedef struct mqttPublishQueueMessage {
    topicType topic_type;
    int event;
    char payload[BUFFER_SIZE];
} mqttPublishQueueMessage;

void createMqttPublishQueue();

mqttPublishQueueMessage receiveFromMqttPublishQueue();

void sendToMqttPublishQueue(mqttPublishQueueMessage* targetMessage);

#endif /* MQTT_PUBLISH_QUEUE_H_ */
