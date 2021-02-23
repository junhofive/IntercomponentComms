/*
 * debug.h
 *
 *  Created on: Feb 10, 2021
 *      Author: Junho Oh
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#define BIT_6   0x40
#define BIT_5   0x20
#define BIT_4   0x10
#define BIT_3   0x08
#define BIT_2   0x04
#define BIT_1   0x02
#define BIT_0   0x01

/* Main Thread */
#define ENTER_MAIN_THREAD                   0x01

/* Initialization Errors */
#define SL_NET_INIT_FAILED                  0x0a
#define WIFI_INIT_FAILED                    0x0b
#define MQTT_INIT_FAILED                    0x0c
#define MQTT_SUBSCRIPTION_FAILED            0x0d
#define MQTT_CONNECT_FAILED                 0x0e

/* Timer 70 */
#define ENTER_TIMER70                       0x10
#define LEAVE_TIMER70                       0x19

/* Timer 500 */
#define ENTER_TIMER500                      0x1a
#define LEAVE_TIMER500                      0x1f

/* Sensor Task */
#define ENTER_SENSOR_TASK                   0x20
#define BEFORE_SENSOR_LOOP                  0x21

/* UART Task */
#define ENTER_UART_TASK                     0x25
#define BEFORE_UART_LOOP                    0x26
#define ENTER_WRITE_TO_UART                 0x28
#define LEAVE_WRITE_TO_UART                 0x29

/* Sensor FSM */
#define ENTER_STATE_MACHINE                 0x2a
#define INIT_AVERAGE_STATE                  0x2b
#define UPDATE_AVERAGE_STATE                0x2c

#define LEAVE_INIT_AVERAGE_STATE            0x2d
#define LEAVE_UPDATE_AVERAGE_STATE          0x2e
#define LEAVE_STATE_MACHINE                 0x2f

/* Sensor Queue */
#define BEFORE_RECEIVE_SENSOR_QUE           0x31
#define AFTER_RECEIVE_SENSOR_QUE            0x39

#define BEFORE_SEND_TIMER70_MSG             0x3a
#define AFTER_SEND_TIMER70_MSG              0x3b

#define BEFORE_SEND_TIMER500_MSG            0x3e
#define AFTER_SEND_TIMER500_MSG             0x3f

/* UART Queue */
#define BEFORE_RECEIVE_UART_QUEUE           0x41
#define AFTER_RECEIVE_UART_QUEUE            0x49

#define BEFORE_SEND_TYPE70_MSG              0x4a
#define AFTER_SEND_TYPE70_MSG               0x4b

#define BEFORE_SEND_TYPE500_MSG             0x4e
#define AFTER_SEND_TYPE500_MSG              0x4f

/* Sensor Queue Errors */
#define SENSOR_QUEUE_NOT_CREATED            0x60
#define SENSOR_QUEUE_NOT_RECEIVED           0x61
#define SENSOR_QUEUE_NOT_SENT               0x62

/* UART Queue Errors */
#define UART_QUEUE_NOT_CREATED              0x63
#define UART_QUEUE_NOT_RECEIVED             0x64
#define UART_QUEUE_NOT_SENT                 0x65

/* MQTT Queue Errors */
#define MQTT_QUEUE_NOT_CREATED              0x67
#define MQTT_QUEUE_NOT_RECEIVED             0x68
#define MQTT_QUEUE_NOT_SENT                 0x69

/* UART Write Error */
#define UART_OPEN_ERROR                     0x70
#define UART_WRITE_ERROR                    0x71

/* Timer Errors*/
#define TIMER0_NOT_OPENED                   0x73
#define TIMER500_NOT_CREATED                0x75
#define TIMER70_NOT_CREATED                 0x77
#define ADC_NOT_OPEN                        0x79

/* pthread related errors */
#define PTHREAD_SET_ATTR_FAILED_MAIN_C      0x7a
#define PTHREAD_CREATE_FAILED_MAIN_C        0x7b
#define PTHREAD_NOT_CREATED                 0x7c

void dbgEvent(unsigned int event);
void handleFatalError(unsigned int eventLabel);
#endif /* DEBUG_H_ */
