/*
 * timer70.c
 * Editor: Tianyu Li, Junho Oh
 */

/*
        Referred to example code timerled.c, adcsinglechannel.c
        Referred to struct in c document: tutorialspoint.com/cprogramming/c_structures.htm
        Referred to documents: Mastering the FreeRTOS Real Time Kernel & The FreeRTOS Reference Manual. URL: https://www.freertos.org/Documentation/RTOS_book.html
        Referred to document: Pointer to a Structure in C. URL: https://overiq.com/c-programming-101/pointer-to-a-structure-in-c/
*/

/*
    Blocking call to the ADC to read a single value from a sensor that you have connected to a single ADC channel.
*/

#include "timer70.h"
#include "sensor_thread_queue.h"
#include <FreeRTOS.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <queue.h>
#include "debug.h"

#include <ti/drivers/ADC.h>
#include <ti/drivers/Timer.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"
#include <ti/drivers/dpl/HwiP.h>

void timer70Callback(Timer_Handle myHandle, int_fast16_t status);

#if 0
int distanceTable(uint32_t microVolt) {
    int distance = -1;

    if (microVolt > 900000) { // 10cm
        distance = 100;
    }
    else if (microVolt > 600000) {
        distance = 150;
    }
    else if (microVolt > 400000) {
        distance = 200;
    }
    else if (microVolt >)

}
#endif

/*
 * This function is used to convert the sensor reading to mm
 */
int convert_to_mm(ADC_Handle adc){
    int_fast16_t res;
    uint16_t adcValue0;
    uint32_t adcValue0MicroVolt;

    int distance;
    float voltage_ratio;
    // Blocking mode conversion
    res = ADC_convert(adc, &adcValue0);

    if (res == ADC_STATUS_SUCCESS) {
        adcValue0MicroVolt = ADC_convertRawToMicroVolts(adc, adcValue0);
    }
    else {  // convert failed
//        handleFatalError(ADC_CONVERSION_ERROR);
    }

    // Knowing the voltage, convert the voltage to distance
    // Formula on this website: https://www.phidgets.com/?tier=3&catid=5&pcid=3&prodid=70
    // Using Sharp 10-80 cm sensor
    voltage_ratio = (float)adcValue0MicroVolt / 5000000;    // 5V input

    distance = (2.076 / (voltage_ratio - 0.011 )) * 10;

    // See if the distance is out of range
    if ( (distance < 100)||(distance > 800) ){
        distance = -1;
    }

    return distance;
}

void *timer70Thread(void *arg0){
    Timer_Handle timer70;
    Timer_Params params;

    Timer_Params_init(&params);
    params.period = 70000; // 70000 microsecond = 70 ms
    params.periodUnits = Timer_PERIOD_US;
    params.timerMode = Timer_CONTINUOUS_CALLBACK;
    params.timerCallback = timer70Callback;

    timer70 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer70 == NULL) {
       /* Failed to initialized timer */
       handleFatalError(TIMER70_NOT_CREATED);
    }

    if (Timer_start(timer70) == Timer_STATUS_ERROR) {
       /* Failed to start timer */
        handleFatalError(TIMER70_NOT_OPENED);
    }

    return (NULL);
}

/*
 * This is the callback function for timer 70.
 * Period = 70 ms
 */
void timer70Callback(Timer_Handle myHandle, int_fast16_t status)
{
    // try to read the output from IR sensor
    // Read the ADC output
    // Using pin 59, CONFIG_ADC_0
    // blocking call: portMAX_DELAY
    dbgEvent(ENTER_TIMER70);
    ADC_Handle  adc;
    ADC_Params  params;
    int         distance;
    SensorThreadMessage message;

    ADC_Params_init(&params);

    adc = ADC_open(CONFIG_ADC_0, &params);

    // error initializing CONFIG_ADC_0
    if (adc == NULL) {
        handleFatalError(ADC_NOT_OPEN);
    }

    distance = convert_to_mm(adc);
    message.value = distance;
    message.message_type = TIMER70_MESSAGE;

    ADC_close(adc);
    // send to message queue
    dbgEvent(BEFORE_SEND_TIMER70_MSG);
    sendToSensorThreadQueueFromISR(&message);
    dbgEvent(AFTER_SEND_TIMER70_MSG);

    dbgEvent(LEAVE_TIMER70);
}
