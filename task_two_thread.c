/*
 * task_two_thread.c
 *
 *  Created on: Feb 28, 2021
 *      Author: JO_Desktop
 */

#include "task_two_thread.h"
#include "task_two_queue.h"
#include "mqtt_publish_queue.h"
#include "str_converter.h"
#include <stdio.h>
#include <string.h>
#include "debug.h"

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};

void *task_two(void *arg0) {
    taskTwoQueueMessage receivedMsg;
    mqttPublishQueueMessage msgToSend;
    int checksum;
    while (1) {
        receivedMsg = receiveFromTaskTwoQueue();


        msgToSend.event = APP_MQTT_PUBLISH;
        msgToSend.topic_type = TASK_TWO_TOPIC;
        checksum = strToSum("Value", strlen("Value")) + (receivedMsg.value * 2);
        snprintf(msgToSend.payload, BUFFER_SIZE, "{\"Value\": %d, \"Checksum\": %d}",
                 receivedMsg.value * 2, checksum);
        sendToMqttPublishQueue(&msgToSend);

    }

}
