/*
 * statistics_task.c
 *
 *  Created on: Mar 9, 2021
 *      Author: JO_Desktop
 */

#include <FreeRTOS.h>
#include "statistics_task.h"
#include "mqtt_publish_queue.h"
#include "statistics_queue.h"
#include "str_converter.h"

#include <ti/drivers/Timer.h>
#include "ti_drivers_config.h"
#include <stdio.h>
#include "debug.h"
#include <string.h>
#include "debug_if.h"

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};

static int totalTaskOneSent,
            totalTaskTwoSent,
            J_sequence,
            T_sequence,
            Chain_sequence,
            receiveTotal;

void *task_statistics(void *arg0) {
    totalTaskOneSent = 0;
    totalTaskTwoSent = 0;
    J_sequence = 0;
    T_sequence = 0;
    Chain_sequence = 0;
    receiveTotal = 0;
    static statisticsQueueMessage receivedMsg;
    static mqttPublishQueueMessage msgToSend;
    static int checksum;

    while(1) {
        receivedMsg = receiveFromStatisticsQueue();
        if (receivedMsg.stat_type == TASK_ONE_STAT) {
            totalTaskOneSent++;

            J_sequence = receivedMsg.J_sequence;
            T_sequence = receivedMsg.T_sequence;

            receiveTotal += receivedMsg.J_MsgCnt;
            receiveTotal += receivedMsg.T_MsgCnt;

            if (receivedMsg.timer_count == 5) {
                msgToSend.event = APP_MQTT_PUBLISH;
                msgToSend.topic_type = STATUS_TOPIC;
                checksum = strToSum("PubAtmpt", strlen("PubAtmpt")) + (totalTaskOneSent + totalTaskTwoSent);
                checksum += strToSum("PubSucs", strlen("PubSucs")) + (J_sequence + T_sequence + Chain_sequence);
                checksum += strToSum("PubRecv", strlen("PubRecv")) + receiveTotal;
                checksum += strToSum("Missing", strlen("Missing"))
                        + ((J_sequence + T_sequence + Chain_sequence) - receiveTotal);
                snprintf(msgToSend.payload, BUFFER_SIZE,
                         "{\"PubAtmpt\": %d, \"PubSucs\": %d, "
                         "\"PubRecv\": %d, \"Missing\": %d, \"Checksum\": %d}",
                         totalTaskOneSent + totalTaskTwoSent,
                         (J_sequence + T_sequence + Chain_sequence),
                         receiveTotal,
                         (J_sequence + T_sequence + Chain_sequence) - receiveTotal, checksum);
                sendToMqttPublishQueue(&msgToSend);
            }
        }
        else if (receivedMsg.stat_type == TASK_TWO_STAT) {
            totalTaskTwoSent++;
            receiveTotal++;
            Chain_sequence = receivedMsg.ChainCount;
        }
    }
}



#if 0
static int totalTaskOneSent,
            totalTaskTwoSent,
            receiveSuccess,
            receiveTotal;

void *task_statistics(void *arg0) {
    totalTaskOneSent = 0;
    totalTaskTwoSent = 0;
    receiveSuccess = 0;
    receiveTotal = 0;
    static statisticsQueueMessage receivedMsg;
    static mqttPublishQueueMessage msgToSend;
    static int checksum;

    while(1) {
        receivedMsg = receiveFromStatisticsQueue();
        if (receivedMsg.stat_type == TASK_ONE_STAT) {
            totalTaskOneSent++;
            if (receivedMsg.timer_count == 10) {
                msgToSend.event = APP_MQTT_PUBLISH;
                msgToSend.topic_type = STATUS_TOPIC;
                checksum = strToSum("PubAtmpt", strlen("PubAtmpt")) + (totalTaskOneSent + totalTaskTwoSent);
                checksum += strToSum("PubSucs", strlen("PubSucs")) + receiveSuccess;
                checksum += strToSum("PubRecv", strlen("PubRecv")) + receiveTotal;
                checksum += strToSum("Missing", strlen("Missing")) + (receiveSuccess - receiveTotal);
                snprintf(msgToSend.payload, BUFFER_SIZE,
                         "{\"PubAtmpt\": %d, \"PubSucs\": %d, "
                         "\"PubRecv\": %d, \"Missing\": %d, \"Checksum\": %d}",
                         totalTaskOneSent + totalTaskTwoSent,
                         receiveSuccess,
                         receiveTotal,
                         receiveSuccess - receiveTotal, checksum);
                sendToMqttPublishQueue(&msgToSend);
            }
        }
        else if (receivedMsg.stat_type == TASK_TWO_STAT) {
            totalTaskTwoSent++;
            receiveTotal++;
            receiveSuccess = receivedMsg.sequence;
        }
    }
}
#endif
