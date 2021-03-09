/*
 * statistics_task.c
 *
 *  Created on: Mar 9, 2021
 *      Author: JO_Desktop
 */

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

void timer10sCallback(Timer_Handle myHandle, int_fast16_t status);

// Can be modified
//static int MsgCount_Jason, MsgCount_Terry, SensorCnt_Jason, SensorCnt_Terry, SensorAvg_Jason, SensorAvg_Terry;
static int totalTaskOneSent, totalTaskTwoSent, receiveSuccess, receiveTotal;

void *task_statistics(void *arg0) {
    Timer_Handle timer10s;
    Timer_Params params;

    Timer_Params_init(&params);
    params.period = 10000000; // 10000000 microsecond = 10s
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timer10sCallback;

//    MsgCount_Jason = 0;
//    MsgCount_Terry = 0;
//    SensorCnt_Jason = 0;
//    SensorCnt_Terry = 0;
//    SensorAvg_Jason = 0;
//    SensorAvg_Terry = 0;


    // For Ajay, he needs to have two of each for Jason and Terry
    totalTaskOneSent = 0;
    totalTaskTwoSent = 0;
    receiveSuccess = 0;
    receiveTotal = 0;

    timer10s = Timer_open(CONFIG_TIMER_2, &params); // 2 for J/T; 1 for Ajay

    if (timer10s == NULL) {
        /* Failed to initialized timer */
        //handleFatalError(TIMER_NOT_INITIALIZED);
    }

    if (Timer_start(timer10s) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        //handleFatalError(TIMER_NOT_OPEN);
    }

    static statisticsQueueMessage receivedMsg;

    while(1) {
        receivedMsg = receiveFromStatisticsQueue();
//        LOG_INFO("STAT\r\n");
        if (receivedMsg.stat_type == TASK_ONE_STAT) {
            totalTaskOneSent++;
        }
        else if (receivedMsg.stat_type == TASK_TWO_STAT) {
            totalTaskTwoSent++;
            receiveTotal++;
            receiveSuccess = receivedMsg.ChainCount;
        }
    }

//        if (receivedMsg.message_type == TIMER70_MSG_JASON) {
//            SensorCnt_Jason = receivedMsg.SensorCount;
//        }
//        else if (receivedMsg.message_type == TIMER500_MSG_JASON) {
//            SensorAvg_Jason += receivedMsg.SensorAvg;
//        }
//        else if (receivedMsg.message_type == TIMER70_MSG_TERRY) {
//            SensorCnt_Terry = receivedMsg.SensorCount;
//        }
//        else if (receivedMsg.message_type == TIMER500_MSG_TERRY) {
//            SensorAvg_Terry += receivedMsg.SensorAvg;
//        }



}

void timer10sCallback(Timer_Handle myHandle, int_fast16_t status){
    mqttPublishQueueMessage msgToSend;
    static int checksum;
    LOG_INFO("STAT_CALLBACK\r\n");
    msgToSend.event = APP_MQTT_PUBLISH;
    msgToSend.event = STATUS_TOPIC;

    // number of times attempting to publish
    // totalTaskOneSent + totalTaskTwoSend
    checksum = strToSum("PubAtmpt", strlen("PubAtmpt")) + (totalTaskOneSent + totalTaskTwoSent);
    checksum += strToSum("PubSucs", strlen("PubSucs")) + receiveSuccess;
    checksum += strToSum("PubRecv", strlen("PubRecv")) + receiveTotal;
    checksum += strToSum("Missing", strlen("Missing")) + (receiveSuccess - receiveTotal);
    snprintf(msgToSend.payload, BUFFER_SIZE,
      "{\"PubAtmpt\": %d, \"PubSucs\": %d, \"PubRecv\": %d, \"Missing\": %d, \"Checksum\": %d}",
      totalTaskOneSent + totalTaskTwoSent,
      receiveSuccess,
      receiveTotal,
      receiveSuccess - receiveTotal, checksum);                  // ChainCount = 5 -> We have should've received until 5th message
                                    // receiveTotal = 4 -> We are missing 1

    sendToMqttPublishQueue(&msgToSend);
}
#if 0
    // Need to publish to a topic every 1s.
    mqttPublishQueueMessage message;

//    if (SensorCount > 0)
//        SensorAvg = SensorSum / SensorCount;
//    else
//        SensorAvg = 0;



    int checksum = strToSum("MessagesCount", strlen("MessagesCount")) + MessageCount
            + strToSum("ReadingsCount", strlen("ReadingsCount")) + SensorCount
            + strToSum("SensorAvg", strlen("SensorAvg")) + SensorAvg;
    message.event = APP_MQTT_PUBLISH;
    message.topic_type = TASK_ONE_TOPIC;
    snprintf(message.payload, BUFFER_SIZE,
             "{\"MessagesCount\": %d, \"ReadingsCount\": %d, \"SensorAvg\": %d, \"Checksum\": %d}",
             MessageCount, SensorCount, SensorAvg, checksum);
    sendToMqttPublishQueue(&message);

    // After sending to the queue, reset
    MessageCount = 0;
    SensorCount = 0;
    SensorAvg = 0;
#endif
