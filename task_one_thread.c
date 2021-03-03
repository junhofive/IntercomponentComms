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

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};

static int MessageCount, SensorCount, SensorSum, SensorAvg;

void timer1000Callback(Timer_Handle myHandle, int_fast16_t status);

void *task_one(void *arg0) {
    Timer_Handle timer1000;
    Timer_Params params;

    Timer_Params_init(&params);
    params.period = 1000000; // 1000000 microsecond = 1s
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timer1000Callback;

    MessageCount = 0;
    SensorCount = 0;
    SensorSum = 0;
    SensorAvg = 0;

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

        if (receivedMsg.message_type == TIMER70_MSG) {
            MessageCount++;
            SensorCount++;
            SensorSum += receivedMsg.SensorReading;
        }
        else if (receivedMsg.message_type == TIMER500_MSG) {
            MessageCount++;
        }
    }


}

void timer1000Callback(Timer_Handle myHandle, int_fast16_t status){
    // Need to publish to a topic every 1s.
    mqttPublishQueueMessage message;

    if (SensorCount > 0)
        SensorAvg = SensorSum / SensorCount;
    else
        SensorAvg = 0;
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
    SensorSum = 0;
    SensorAvg = 0;
}

#if 0
void *task_one(void *arg0) {
    taskOneQueueMessage receivedMsg;
    mqttPublishQueueMessage msgToSend;
    static int checksum;
    while(1) {
        receivedMsg = receiveFromTaskOneQueue();

        if (receivedMsg.message_type == TIMER70_MSG) {
            msgToSend.event = APP_MQTT_PUBLISH;
            msgToSend.topic_type = TASK_ONE_TOPIC;
            checksum = strToSum("SensorReading", strlen("SensorReading")) + receivedMsg.SensorReading
                    + strToSum("SensorCount", strlen("SensorCount")) + receivedMsg.SensorCount;
            snprintf(msgToSend.payload, BUFFER_SIZE, "{\"SensorReading\": %d, \"SensorCount\": %d, \"Checksum\": %d}",
                     receivedMsg.SensorReading, receivedMsg.SensorCount, checksum);
            sendToMqttPublishQueue(&msgToSend);
        }
        else if (receivedMsg.message_type == TIMER500_MSG) {
            msgToSend.event = APP_MQTT_PUBLISH;
            msgToSend.topic_type = TASK_ONE_TOPIC;
            checksum = strToSum("SensorAvg", strlen("SensorAvg")) + receivedMsg.SensorAvg
                    + strToSum("Time", strlen("Time")) + receivedMsg.Time;
            snprintf(msgToSend.payload, BUFFER_SIZE, "{\"SensorAvg\": %d, \"Time\": %d, \"Checksum\": %d}",
                     receivedMsg.SensorAvg, receivedMsg.Time, checksum);
            sendToMqttPublishQueue(&msgToSend);
        }
    }
}
#endif

