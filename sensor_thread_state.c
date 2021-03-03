/*
 * sensor_thread_state.c
 *
 *  Created on: Feb 9, 2021
 *      Author: JO_Desktop
 */

#include "sensor_thread_state.h"
#include "task_one_queue.h"
#include "sensor_task.h"
#include "debug.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ti/drivers/dpl/HwiP.h>

typedef enum {
    INIT_AVERAGE,
    UPDATE_AVERAGE
} threadState;

static int sensorTotal = 0, sensorCount = 0;
static threadState currentState = INIT_AVERAGE;

void enterStateMachine(SensorThreadMessage *receivedMessage) {
    taskOneQueueMessage message;
    switch (currentState) {
        case INIT_AVERAGE:
            dbgEvent(INIT_AVERAGE_STATE);
            if (receivedMessage->message_type == TIMER500_MESSAGE) {
                currentState = UPDATE_AVERAGE;
                dbgEvent(LEAVE_INIT_AVERAGE_STATE);
            }

            break;
        case UPDATE_AVERAGE:
            dbgEvent(UPDATE_AVERAGE_STATE);
            if (receivedMessage->message_type == TIMER70_MESSAGE) {
                if (receivedMessage->value != NULL) {
                    sensorCount += 1;
                    sensorTotal += receivedMessage->value;
                    message.message_type = TIMER70_MSG;
                    message.SensorReading = receivedMessage->value;
                    message.SensorCount = sensorCount;
                    dbgEvent(BEFORE_SEND_TYPE70_MSG);
                    sendToTaskOneQueue(&message);
                    dbgEvent(AFTER_SEND_TYPE70_MSG);
#if 0
                    mqttPublishQueueMessage message;

                    message.event = APP_MQTT_PUBLISH;
                    snprintf(message.payload, BUFFER_SIZE, "{\"SensorReading\": %d, \"SensorCount\": %d}", receivedMessage->value, sensorCount);
                    dbgEvent(BEFORE_SEND_TYPE70_MSG);
                    sendToMqttPublishQueue(&message);
                    dbgEvent(AFTER_SEND_TYPE70_MSG);
#endif
                }
            }
            if (receivedMessage->message_type == TIMER500_MESSAGE) {
                int averageSensorValue = 0;

                if (sensorCount != 0) {
                    averageSensorValue = sensorTotal / sensorCount;
                }
                message.message_type = TIMER500_MSG;
                message.SensorAvg = averageSensorValue;
                message.Time = receivedMessage->value;
                dbgEvent(BEFORE_SEND_TYPE500_MSG);
                sendToTaskOneQueue(&message);
                dbgEvent(AFTER_SEND_TYPE500_MSG);
#if 0
                mqttPublishQueueMessage message;
                message.event = APP_MQTT_PUBLISH;
                snprintf(message.payload, BUFFER_SIZE, "{\"SensorAvg\": %d, \"Time\": %d}", averageSensorValue, receivedMessage->value);
                dbgEvent(BEFORE_SEND_TYPE500_MSG);
                sendToMqttPublishQueue(&message);
                dbgEvent(AFTER_SEND_TYPE500_MSG);
#endif
                dbgEvent(LEAVE_UPDATE_AVERAGE_STATE);
                currentState = INIT_AVERAGE;

            }

            break;
    }
}
