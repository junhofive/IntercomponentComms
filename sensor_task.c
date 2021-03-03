/*
 * sensor_task.c
 *
 *  Created on: Feb 6, 2021
 *      Author: JO_Desktop
 */

#include "sensor_task.h"
#include "sensor_thread_queue.h"
#include "sensor_thread_state.h"
#include "debug.h"

#include <stdint.h>
#include <stddef.h>

void *sensor_task(void *arg0) {
    dbgEvent(ENTER_SENSOR_TASK);
    SensorThreadMessage sensorMessage;

    dbgEvent(BEFORE_SENSOR_LOOP);
    while(1) {
        dbgEvent(BEFORE_RECEIVE_SENSOR_QUE);
        sensorMessage = receiveFromSensorThreadQueue();
        dbgEvent(AFTER_RECEIVE_SENSOR_QUE);

        dbgEvent(ENTER_STATE_MACHINE);
        enterStateMachine(&sensorMessage);
        dbgEvent(LEAVE_STATE_MACHINE);
    }
}
