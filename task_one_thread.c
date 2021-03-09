/*
 * task_one_thread.c
 *
 *  Created on: Mar 1, 2021
 *      Author: JO_Desktop
 */

#include "task_one_thread.h"
#include "mqtt_publish_queue.h"
#include "task_one_queue.h"
#include "str_converter.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <ti/drivers/Timer.h>
#include "ti_drivers_config.h"
#include "debug_if.h"

#include "statistics_queue.h"

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};
#if 0
static int MsgCount_Jason, MsgCount_Terry, SensorCnt_Jason, SensorCnt_Terry, SensorAvg_Jason, SensorAvg_Terry;

void timer1000Callback(Timer_Handle myHandle, int_fast16_t status);

void *task_one(void *arg0) {
    Timer_Handle timer1000;
    Timer_Params params;

    Timer_Params_init(&params);
    params.period = 1000000; // 1000000 microsecond = 1s
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timer1000Callback;

    MsgCount_Jason = 0;
    MsgCount_Terry = 0;
    SensorCnt_Jason = 0;
    SensorCnt_Terry = 0;
    SensorAvg_Jason = 0;
    SensorAvg_Terry = 0;

    timer1000 = Timer_open(CONFIG_TIMER_0, &params);
    if (timer1000 == NULL) {
        /* Failed to initialized timer */
        //handleFatalError(TIMER_NOT_INITIALIZED);
    }

    if (Timer_start(timer1000) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        //handleFatalError(TIMER_NOT_OPEN);
    }

    taskOneQueueMessage receivedMsg;

    while(1) {
        receivedMsg = receiveFromTaskOneQueue();

        if (receivedMsg.message_type == TIMER70_MSG_JASON) {
            SensorCnt_Jason = receivedMsg.SensorCount;
        }
        else if (receivedMsg.message_type == TIMER500_MSG_JASON) {
            SensorAvg_Jason += receivedMsg.SensorAvg;
        }
        else if (receivedMsg.message_type == TIMER70_MSG_TERRY) {
            SensorCnt_Terry = receivedMsg.SensorCount;
        }
        else if (receivedMsg.message_type == TIMER500_MSG_TERRY) {
            SensorAvg_Terry += receivedMsg.SensorAvg;
        }
    }


}

void timer1000Callback(Timer_Handle myHandle, int_fast16_t status){
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
}
#endif

void *task_one(void *arg0) {
    dbgEvent(ENTER_TASK_ONE);
    static taskOneQueueMessage receivedMsg;
    mqttPublishQueueMessage msgToSend;
//    statisticsQueueMessage statusMsg;

    static int checksum;
    dbgEvent(BEFORE_TASK_ONE_LOOP);
    while(1) {
        dbgEvent(BEFORE_RECEIVE_TASK_ONE_MSG);
        receivedMsg = receiveFromTaskOneQueue();
        dbgEvent(AFTER_RECEIVE_TASK_ONE_MSG);

        msgToSend.event = APP_MQTT_PUBLISH;
        msgToSend.topic_type = TASK_ONE_TOPIC;

//        statusMsg.stat_type = TASK_ONE_STAT;

        if (receivedMsg.message_type == TIMER70_MSG) {
            checksum = strToSum("SensorReading", strlen("SensorReading")) + receivedMsg.SensorReading
                    + strToSum("SensorCount", strlen("SensorCount")) + receivedMsg.SensorCount;
            snprintf(msgToSend.payload, BUFFER_SIZE, "{\"SensorReading\": %d, \"SensorCount\": %d, \"Checksum\": %d}",
                     receivedMsg.SensorReading, receivedMsg.SensorCount, checksum);
        }
        else if (receivedMsg.message_type == TIMER500_MSG) {
            checksum = strToSum("SensorAvg", strlen("SensorAvg")) + receivedMsg.SensorAvg
                    + strToSum("Time", strlen("Time")) + receivedMsg.Time;
            snprintf(msgToSend.payload, BUFFER_SIZE, "{\"SensorAvg\": %d, \"Time\": %d, \"Checksum\": %d}",
                     receivedMsg.SensorAvg, receivedMsg.Time, checksum);
        }
        dbgEvent(BEFORE_SEND_TASK_ONE_MSG_TO_MQTT);
        sendToMqttPublishQueue(&msgToSend);
        dbgEvent(AFTER_SEND_TASK_ONE_MSG_TO_MQTT);

//        sendToStatisticsQueue(&statusMsg);
    }
}

// Sensor Count - Jason
// Sensor Count - Terry
