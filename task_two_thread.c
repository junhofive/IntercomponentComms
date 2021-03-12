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
#include "statistics_queue.h"
#include "debug_if.h"

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};



void *task_two(void *arg0) {
    dbgEvent(ENTER_TASK_TWO);
    static taskTwoQueueMessage receivedMsg;
    static mqttPublishQueueMessage msgToSend;
    static statisticsQueueMessage statusMsg;

    static int checksum;
    dbgEvent(BEFORE_TASK_TWO_LOOP);
    while (1) {
        dbgEvent(BEFORE_RECEIVE_TASK_TWO_MSG);
        receivedMsg = receiveFromTaskTwoQueue();
        dbgEvent(AFTER_RECEIVE_TASK_TWO_MSG);

        msgToSend.event = APP_MQTT_PUBLISH;
        msgToSend.topic_type = TASK_TWO_TOPIC;

        statusMsg.stat_type = TASK_TWO_STAT;
        statusMsg.ChainCount = receivedMsg.ChainCount;

        checksum = strToSum("Value", strlen("Value")) + (receivedMsg.value + 1);
        checksum += strToSum("ChainCount", strlen("ChainCount")) + receivedMsg.ChainCount;
        snprintf(msgToSend.payload, BUFFER_SIZE, "{\"Value\": %d, \"ChainCount\": %d, \"Checksum\": %d}",
                 receivedMsg.value + 1, receivedMsg.ChainCount, checksum);
        dbgEvent(BEFORE_SEND_TASK_TWO_MSG_TO_MQTT);
        sendToMqttPublishQueue(&msgToSend);
        dbgEvent(AFTER_SEND_TASK_TWO_MSG_TO_MQTT);

        dbgEvent(BEFORE_SEND_TO_STATISTICS);
        sendToStatisticsQueue(&statusMsg);
        dbgEvent(AFTER_SEND_TO_STATISTICS);

    }

}
