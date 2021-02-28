/*
 * debug.c
 *
 *  Created on: Feb 9, 2021
 *      Author: Junho Oh
 */

// Refer to decimal to binary document:C Program to convert Decimal to Binary, URL: https://www.javatpoint.com/c-program-to-convert-decimal-to-binary

#include "debug.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"
#include <ti/drivers/Power.h>
#include <ti/drivers/UART.h>
#include <FreeRTOS.h>
#include <FreeRTOSConfig.h>
#include <task.h>
#include <ti/drivers/dpl/HwiP.h>

void dbgEvent(unsigned int event) {
    if (event <= 127){
        GPIO_write(CONFIG_GPIO_7, 1);               // P50/30

        GPIO_write(CONFIG_GPIO_6, event & BIT_6);   // P21/13

        GPIO_write(CONFIG_GPIO_5, event & BIT_5);   // P18/19

        GPIO_write(CONFIG_GPIO_4, event & BIT_4);   // P53/28

        GPIO_write(CONFIG_GPIO_3, event & BIT_3);   // P61/5

        GPIO_write(CONFIG_GPIO_2, event & BIT_2);   // P62/8

        GPIO_write(CONFIG_GPIO_1, event & BIT_1);   // P63/27

        GPIO_write(CONFIG_GPIO_0, event & BIT_0);   // P03/4

        GPIO_write(CONFIG_GPIO_7, 0);
    }
    else{
        handleFatalError(event);
    }
}

void handleFatalError(unsigned int eventLabel) {


    /* Disable hardware interrupts */
    HwiP_disable();

    /* Disable threads */
    vTaskSuspendAll();

    dbgEvent(eventLabel);

    while (1) {
        GPIO_toggle(CONFIG_GPIO_LED_0);
    }
}

