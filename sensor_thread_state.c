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
    static taskOneQueueMessage message;
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
                }
            }
            if (receivedMessage->message_type == TIMER500_MESSAGE) {
                message.message_type = TIMER500_MSG;
                if (sensorCount != 0)
                    message.SensorAvg = sensorTotal / sensorCount;
                else
                    message.SensorAvg = 0;
                message.Time = receivedMessage->value;
                dbgEvent(BEFORE_SEND_TYPE500_MSG);
                sendToTaskOneQueue(&message);
                dbgEvent(AFTER_SEND_TYPE500_MSG);

                dbgEvent(LEAVE_UPDATE_AVERAGE_STATE);
                currentState = INIT_AVERAGE;

            }

            break;
    }
}
