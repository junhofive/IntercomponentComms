/*
 * sensor_thread_state.h
 *
 *  Created on: Feb 9, 2021
 *      Author: JO_Desktop
 */

#ifndef SENSOR_THREAD_STATE_H_
#define SENSOR_THREAD_STATE_H_
#include "sensor_thread_queue.h"

void enterStateMachine(SensorThreadMessage *receivedMessage);

#endif /* SENSOR_THREAD_STATE_H_ */
