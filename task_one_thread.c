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

static int MsgCount_Jason,
            MsgCount_Terry,
            SensorCnt_Jason,
            SensorCnt_Terry,
            SensorAvg_Jason,
            SensorAvg_Terry,
            Sequence_Jason,
            Sequence_Terry,
            timer_count;

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
    Sequence_Jason = 0;
    Sequence_Terry = 0;
    timer_count = 0;

    timer1000 = Timer_open(CONFIG_TIMER_0, &params);
    if (timer1000 == NULL) {
        /* Failed to initialized timer */
        handleFatalError(TIMER500_NOT_CREATED);
    }

    if (Timer_start(timer1000) == Timer_STATUS_ERROR) {
        /* Failed to start timer */
        handleFatalError(TIMER500_NOT_OPENED);
    }

    static taskOneQueueMessage receivedMsg;

    while(1) {
        receivedMsg = receiveFromTaskOneQueue();

        if (receivedMsg.message_type == TIMER70_JASON) {
            MsgCount_Jason++;
            SensorCnt_Jason = receivedMsg.SensorCount;
            Sequence_Jason = receivedMsg.Sequence;
        }
        else if (receivedMsg.message_type == TIMER500_JASON) {
            MsgCount_Jason++;
            SensorAvg_Jason += receivedMsg.SensorAvg;
            Sequence_Jason = receivedMsg.Sequence;
        }
        else if (receivedMsg.message_type == TIMER70_TERRY) {
            MsgCount_Terry++;
            SensorCnt_Terry = receivedMsg.SensorCount;
            Sequence_Terry = receivedMsg.Sequence;
        }
        else if (receivedMsg.message_type == TIMER500_TERRY) {
            MsgCount_Terry++;
            SensorAvg_Terry += receivedMsg.SensorAvg;
            Sequence_Terry = receivedMsg.Sequence;
        }
    }


}

void timer1000Callback(Timer_Handle myHandle, int_fast16_t status){
    // Need to publish to a topic every 1s.
    static mqttPublishQueueMessage message;
    static statisticsQueueMessage statusMsg;
    static int checksum = 0;

    if ((MsgCount_Jason - SensorCnt_Jason) > 0)
        SensorAvg_Jason /= (MsgCount_Jason - SensorCnt_Jason);
    else
        SensorAvg_Jason = 0;

    if ((MsgCount_Terry - SensorCnt_Terry) > 0)
        SensorAvg_Terry /= (MsgCount_Terry - SensorCnt_Terry);
    else
        SensorAvg_Terry = 0;

    timer_count++;

    checksum = strToSum("J_MsgCnt", strlen("J_MsgCnt")) + MsgCount_Jason;
    checksum += strToSum("T_MsgCnt", strlen("T_MsgCnt")) + MsgCount_Terry;
    checksum += strToSum("J_ReadCnt", strlen("J_ReadCnt")) + SensorCnt_Jason;
    checksum += strToSum("T_ReadCnt", strlen("T_ReadCnt")) + SensorCnt_Terry;
    checksum += strToSum("J_Avg", strlen("J_Avg")) + SensorAvg_Jason;
    checksum += strToSum("T_Avg", strlen("T_Avg")) + SensorAvg_Terry;

    message.event = APP_MQTT_PUBLISH;
    message.topic_type = TASK_ONE_TOPIC;
    snprintf(message.payload, BUFFER_SIZE,
             "{\"J_MsgCnt\": %d, \"T_MsgCnt\": %d, "
             "\"J_ReadCnt\": %d, \"T_ReadCnt\": %d, "
             "\"J_Avg\": %d, \"T_Avg\": %d, \"Checksum\": %d}",
             MsgCount_Jason, MsgCount_Terry,
             SensorCnt_Jason, SensorCnt_Terry,
             SensorAvg_Jason, SensorAvg_Terry, checksum);
    sendToMqttPublishQueue(&message);

    statusMsg.stat_type = TASK_ONE_STAT;
    statusMsg.J_MsgCnt = MsgCount_Jason;
    statusMsg.T_MsgCnt = MsgCount_Terry;
    statusMsg.J_sequence = Sequence_Jason;
    statusMsg.T_sequence = Sequence_Terry;

    if (timer_count >= 5) {
        statusMsg.timer_count = 5;
        timer_count = 0;
    }
    else
        statusMsg.timer_count = 0;

    sendToStatisticsQueue(&statusMsg);

    // After sending to the queue, reset
    MsgCount_Jason = 0;
    MsgCount_Terry = 0;
    SensorCnt_Jason = 0;
    SensorCnt_Terry = 0;
    SensorAvg_Jason = 0;
    SensorAvg_Terry = 0;
}



#if 0
void *task_one(void *arg0) {
    dbgEvent(ENTER_TASK_ONE);
    static taskOneQueueMessage receivedMsg;
    static mqttPublishQueueMessage msgToSend;
    static statisticsQueueMessage statusMsg;

    static int timer500count = 0;
    static int checksum;
    static int msg_sequence = 0;

    dbgEvent(BEFORE_TASK_ONE_LOOP);
    while(1) {
        dbgEvent(BEFORE_RECEIVE_TASK_ONE_MSG);
        receivedMsg = receiveFromTaskOneQueue();
        dbgEvent(AFTER_RECEIVE_TASK_ONE_MSG);

        msg_sequence++;

        msgToSend.event = APP_MQTT_PUBLISH;
        msgToSend.topic_type = TASK_ONE_TOPIC;

        statusMsg.stat_type = TASK_ONE_STAT;


        if (receivedMsg.message_type == TIMER70_MSG) {
            checksum = strToSum("SensorReading", strlen("SensorReading")) + receivedMsg.SensorReading
                    + strToSum("SensorCount", strlen("SensorCount")) + receivedMsg.SensorCount
                    + strToSum("Sequence", strlen("Sequence")) + msg_sequence;
            snprintf(msgToSend.payload, BUFFER_SIZE,
                     "{\"SensorReading\": %d, \"SensorCount\": %d, "
                     "\"Sequence\": %d, \"Checksum\": %d}",
                     receivedMsg.SensorReading, receivedMsg.SensorCount, msg_sequence, checksum);
            statusMsg.timer_count = 0;
        }
        else if (receivedMsg.message_type == TIMER500_MSG) {
            checksum = strToSum("SensorAvg", strlen("SensorAvg")) + receivedMsg.SensorAvg
                    + strToSum("Time", strlen("Time")) + receivedMsg.Time
                    + strToSum("Sequence", strlen("Sequence")) + msg_sequence;
            snprintf(msgToSend.payload, BUFFER_SIZE,
                     "{\"SensorAvg\": %d, \"Time\": %d, \"Sequence\": %d, \"Checksum\": %d}",
                     receivedMsg.SensorAvg, receivedMsg.Time, msg_sequence, checksum);

            timer500count++;
            if (timer500count >= 9) {
                statusMsg.timer_count = 10;
                timer500count = 0;
            }
            else {
                statusMsg.timer_count = 0;
            }
        }
        dbgEvent(BEFORE_SEND_TASK_ONE_MSG_TO_MQTT);
        sendToMqttPublishQueue(&msgToSend);
        dbgEvent(AFTER_SEND_TASK_ONE_MSG_TO_MQTT);


        sendToStatisticsQueue(&statusMsg);
    }
}
#endif
