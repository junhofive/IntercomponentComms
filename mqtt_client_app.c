/*
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*****************************************************************************

   Application Name     -   MQTT Client
   Application Overview -   The device is running a MQTT client which is
                           connected to the online broker. Three LEDs on the
                           device can be controlled from a web client by
                           publishing msg on appropriate topics. Similarly,
                           message can be published on pre-configured topics
                           by pressing the switch buttons on the device.

   Application Details  - Refer to 'MQTT Client' README.html

*****************************************************************************/
#include <mqtt_if.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <mqueue.h>

#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/slnetifwifi.h>

#include <ti/drivers/SPI.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/ADC.h>
#include <ti/drivers/Timer.h>

#include <ti/net/mqtt/mqttclient.h>

#include "network_if.h"
#include "uart_term.h"
#include "mqtt_if.h"
#include "debug_if.h"
#include "debug.h"
#include "ti_drivers_config.h"
#include "sensor_task.h"
#include "timer70.h"
#include "timer500.h"
#include "sensor_thread_queue.h"
#include "mqtt_publish_queue.h"
#include "task_one_queue.h"
#include "task_one_thread.h"
#include "task_two_queue.h"
#include "task_two_thread.h"
#include "statistics_queue.h"
#include "statistics_task.h"

#define THREADSTACKSIZE   (896)
//#define THREADSTACKSIZE_SM (768)

extern int32_t ti_net_SlNet_initConfig();

#define APPLICATION_NAME         "MQTT client"
#define APPLICATION_VERSION      "2.0.0"

#define SL_TASKSTACKSIZE            2048
#define SPAWN_TASK_PRIORITY         9

// un-comment this if you want to connect to an MQTT broker securely
//#define MQTT_SECURE_CLIENT

#define MQTT_MODULE_TASK_PRIORITY   2
#define MQTT_MODULE_TASK_STACK_SIZE 2048

#define MQTT_WILL_TOPIC             "JasonBoardWill"
#define MQTT_WILL_MSG               "Disconnected"
#define MQTT_WILL_QOS               MQTT_QOS_0
#define MQTT_WILL_RETAIN            false

#define MQTT_CLIENT_PASSWORD        "rzxe7rl2"
#define MQTT_CLIENT_USERNAME        "wg3z4rvo"
#define MQTT_CLIENT_KEEPALIVE       0
#define MQTT_CLIENT_CLEAN_CONNECT   true
#define MQTT_CLIENT_MQTT_V3_1       true
#define MQTT_CLIENT_BLOCKING_SEND   true

#ifndef MQTT_SECURE_CLIENT
#define MQTT_CONNECTION_FLAGS           MQTTCLIENT_NETCONN_URL
#define MQTT_CONNECTION_ADDRESS         "s1.airmqtt.com"
#define MQTT_CONNECTION_PORT_NUMBER     10021
#else
#define MQTT_CONNECTION_FLAGS           MQTTCLIENT_NETCONN_IP4 | MQTTCLIENT_NETCONN_SEC
#define MQTT_CONNECTION_ADDRESS         "s1.airmqtt.com"
#define MQTT_CONNECTION_PORT_NUMBER     10021
#endif

/* Client ID                                                                 */
/* If ClientId isn't set, the MAC address of the device will be copied into  */
/* the ClientID parameter.                                                   */
char ClientId[13] = {'\0'};

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};

MQTT_IF_InitParams_t mqttInitParams =
{
     MQTT_MODULE_TASK_STACK_SIZE,   // stack size for mqtt module - default is 2048
     MQTT_MODULE_TASK_PRIORITY      // thread priority for MQTT   - default is 2
};

MQTTClient_Will mqttWillParams =
{
     MQTT_WILL_TOPIC,    // will topic
     MQTT_WILL_MSG,      // will message
     MQTT_WILL_QOS,      // will QoS
     MQTT_WILL_RETAIN    // retain flag
};

MQTT_IF_ClientParams_t mqttClientParams =
{
     ClientId,                  // client ID
     MQTT_CLIENT_USERNAME,      // user name
     MQTT_CLIENT_PASSWORD,      // password
     MQTT_CLIENT_KEEPALIVE,     // keep-alive time
     MQTT_CLIENT_CLEAN_CONNECT, // clean connect flag
     MQTT_CLIENT_MQTT_V3_1,     // true = 3.1, false = 3.1.1
     MQTT_CLIENT_BLOCKING_SEND, // blocking send flag
     &mqttWillParams            // will parameters
};

#ifndef MQTT_SECURE_CLIENT
MQTTClient_ConnParams mqttConnParams =
{
     MQTT_CONNECTION_FLAGS,         // connection flags
     MQTT_CONNECTION_ADDRESS,       // server address
     MQTT_CONNECTION_PORT_NUMBER,   // port number of MQTT server
     0,                             // method for secure socket
     0,                             // cipher for secure socket
     0,                             // number of files for secure connection
     NULL                           // secure files
};
#else
/*
 * In order to connect to an MQTT broker securely, the MQTTCLIENT_NETCONN_SEC flag,
 * method for secure socket, cipher, secure files, number of secure files must be set
 * and the certificates must be programmed to the file system.
 *
 * The first parameter is a bit mask which configures the server address type and security mode.
 * Server address type: IPv4, IPv6 and URL must be declared with the corresponding flag.
 * All flags can be found in mqttclient.h.
 *
 * The flag MQTTCLIENT_NETCONN_SEC enables the security (TLS) which includes domain name
 * verification and certificate catalog verification. Those verifications can be skipped by
 * adding to the bit mask: MQTTCLIENT_NETCONN_SKIP_DOMAIN_NAME_VERIFICATION and
 * MQTTCLIENT_NETCONN_SKIP_CERTIFICATE_CATALOG_VERIFICATION.
 *
 * Note: The domain name verification requires URL Server address type otherwise, this
 * verification will be disabled.
 *
 * Secure clients require time configuration in order to verify the server certificate validity (date)
 */

/* Day of month (DD format) range 1-31                                       */
#define DAY                      1
/* Month (MM format) in the range of 1-12                                    */
#define MONTH                    5
/* Year (YYYY format)                                                        */
#define YEAR                     2020
/* Hours in the range of 0-23                                                */
#define HOUR                     4
/* Minutes in the range of 0-59                                              */
#define MINUTES                  00
/* Seconds in the range of 0-59                                              */
#define SEC                      00

char *MQTTClient_secureFiles[1] = {"ca-cert.pem"};

MQTTClient_ConnParams mqttConnParams =
{
    MQTT_CONNECTION_FLAGS,                  // connection flags
    MQTT_CONNECTION_ADDRESS,                // server address
    MQTT_CONNECTION_PORT_NUMBER,            // port number of MQTT server
    SLNETSOCK_SEC_METHOD_SSLv3_TLSV1_2,     // method for secure socket
    SLNETSOCK_SEC_CIPHER_FULL_LIST,         // cipher for secure socket
    1,                                      // number of files for secure connection
    MQTTClient_secureFiles                  // secure files
};

void setTime(){

    SlDateTime_t dateTime = {0};
    dateTime.tm_day = (uint32_t)DAY;
    dateTime.tm_mon = (uint32_t)MONTH;
    dateTime.tm_year = (uint32_t)YEAR;
    dateTime.tm_hour = (uint32_t)HOUR;
    dateTime.tm_min = (uint32_t)MINUTES;
    dateTime.tm_sec = (uint32_t)SEC;
    sl_DeviceSet(SL_DEVICE_GENERAL, SL_DEVICE_GENERAL_DATE_TIME,
                 sizeof(SlDateTime_t), (uint8_t *)(&dateTime));
}
#endif

//*****************************************************************************
//!
//! Set the ClientId with its own mac address
//! This routine converts the mac address which is given
//! by an integer type variable in hexadecimal base to ASCII
//! representation, and copies it into the ClientId parameter.
//!
//! \param  macAddress  -   Points to string Hex.
//!
//! \return void.
//!
//*****************************************************************************
int32_t SetClientIdNamefromMacAddress()
{
    int32_t ret = 0;
    uint8_t Client_Mac_Name[2];
    uint8_t Index;
    uint16_t macAddressLen = SL_MAC_ADDR_LEN;
    uint8_t macAddress[SL_MAC_ADDR_LEN];

    /*Get the device Mac address */
    ret = sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, 0, &macAddressLen,
                       &macAddress[0]);

    /*When ClientID isn't set, use the mac address as ClientID               */
    if(ClientId[0] == '\0')
    {
        /*6 bytes is the length of the mac address                           */
        for(Index = 0; Index < SL_MAC_ADDR_LEN; Index++)
        {
            /*Each mac address byte contains two hexadecimal characters      */
            /*Copy the 4 MSB - the most significant character                */
            Client_Mac_Name[0] = (macAddress[Index] >> 4) & 0xf;
            /*Copy the 4 LSB - the least significant character               */
            Client_Mac_Name[1] = macAddress[Index] & 0xf;

            if(Client_Mac_Name[0] > 9)
            {
                /*Converts and copies from number that is greater than 9 in  */
                /*hexadecimal representation (a to f) into ascii character   */
                ClientId[2 * Index] = Client_Mac_Name[0] + 'a' - 10;
            }
            else
            {
                /*Converts and copies from number 0 - 9 in hexadecimal       */
                /*representation into ascii character                        */
                ClientId[2 * Index] = Client_Mac_Name[0] + '0';
            }
            if(Client_Mac_Name[1] > 9)
            {
                /*Converts and copies from number that is greater than 9 in  */
                /*hexadecimal representation (a to f) into ascii character   */
                ClientId[2 * Index + 1] = Client_Mac_Name[1] + 'a' - 10;
            }
            else
            {
                /*Converts and copies from number 0 - 9 in hexadecimal       */
                /*representation into ascii character                        */
                ClientId[2 * Index + 1] = Client_Mac_Name[1] + '0';
            }
        }
    }
    return(ret);
}
#if 0
void timerCallback(Timer_Handle myHandle)
{
    longPress = 1;
}
#endif
// this timer callback toggles the LED once per second until the device connects to an AP
void timerLEDCallback(Timer_Handle myHandle)
{
    GPIO_toggle(CONFIG_GPIO_LED_0);
}

#if 0
void pushButtonPublishHandler(uint_least8_t index)
{
    mqttPublishQueueMessage queueElement;
//    GPIO_disableInt(CONFIG_GPIO_BUTTON_0);
    snprintf(queueElement.payload, 50, "{\"SensorAvg\": %d, \"Time\": Test}");
    queueElement.event = APP_MQTT_PUBLISH;

    sendToMqttPublishQueue(&queueElement);
}
#endif

void MQTT_EventCallback(int32_t event){
    switch(event){

        case MQTT_EVENT_CONNACK:
        {
            LOG_INFO("MQTT_EVENT_CONNACK\r\n");
            break;
        }

        case MQTT_EVENT_SUBACK:
        {
            LOG_INFO("MQTT_EVENT_SUBACK\r\n");
            break;
        }

        case MQTT_EVENT_PUBACK:
        {
            LOG_INFO("MQTT_EVENT_PUBACK\r\n");
            break;
        }

        case MQTT_EVENT_UNSUBACK:
        {
            LOG_INFO("MQTT_EVENT_UNSUBACK\r\n");
            break;
        }

        case MQTT_EVENT_CLIENT_DISCONNECT:
        {
            handleFatalError(MQTT_CLIENT_DISCONNECT);
            break;
        }

        case MQTT_EVENT_SERVER_DISCONNECT:
        {
            handleFatalError(MQTT_SERVER_DISCONNECT);
            break;
        }

        case MQTT_EVENT_DESTROY:
        {
            LOG_INFO("MQTT_EVENT_DESTROY\r\n");
            break;
        }
    }
}

/*
 * Subscribe topic callbacks
 * Topic and payload data is deleted after topic callbacks return.
 * User must copy the topic or payload data if it needs to be saved.
 */
void JasonCB(char* topic, char* payload){
    LOG_INFO("TOPIC: %s \tPAYLOAD: %s\r\n", topic, payload);
}

void TerryCB(char* topic, char* payload){
    LOG_INFO("TOPIC: %s \tPAYLOAD: %s\r\n", topic, payload);
}

void ChainCB(char* topic, char* payload){
    LOG_INFO("TOPIC: %s \tPAYLOAD: %s\r\n", topic, payload);
}

#if 0
void ToggleLED1CB(char* topic, char* payload){
    GPIO_toggle(CONFIG_GPIO_LED_0);
    LOG_INFO("TOPIC: %s \tPAYLOAD: %s\r\n", topic, payload);
}
#endif

int32_t DisplayAppBanner(char* appName, char* appVersion){

    int32_t ret = 0;
    uint8_t macAddress[SL_MAC_ADDR_LEN];
    uint16_t macAddressLen = SL_MAC_ADDR_LEN;
    uint16_t ConfigSize = 0;
    uint8_t ConfigOpt = SL_DEVICE_GENERAL_VERSION;
    SlDeviceVersion_t ver = {0};

    ConfigSize = sizeof(SlDeviceVersion_t);

    // get the device version info and MAC address
    ret = sl_DeviceGet(SL_DEVICE_GENERAL, &ConfigOpt, &ConfigSize, (uint8_t*)(&ver));
    ret |= (int32_t)sl_NetCfgGet(SL_NETCFG_MAC_ADDRESS_GET, 0, &macAddressLen, &macAddress[0]);

    UART_PRINT("\n\r\t============================================\n\r");
    UART_PRINT("\t   %s Example Ver: %s",appName, appVersion);
    UART_PRINT("\n\r\t============================================\n\r\n\r");

    UART_PRINT("\t CHIP: 0x%x\n\r",ver.ChipId);
    UART_PRINT("\t MAC:  %d.%d.%d.%d\n\r",ver.FwVersion[0],ver.FwVersion[1],
               ver.FwVersion[2],
               ver.FwVersion[3]);
    UART_PRINT("\t PHY:  %d.%d.%d.%d\n\r",ver.PhyVersion[0],ver.PhyVersion[1],
               ver.PhyVersion[2],
               ver.PhyVersion[3]);
    UART_PRINT("\t NWP:  %d.%d.%d.%d\n\r",ver.NwpVersion[0],ver.NwpVersion[1],
               ver.NwpVersion[2],
               ver.NwpVersion[3]);
    UART_PRINT("\t ROM:  %d\n\r",ver.RomVersion);
    UART_PRINT("\t HOST: %s\n\r", SL_DRIVER_VERSION);
    UART_PRINT("\t MAC address: %02x:%02x:%02x:%02x:%02x:%02x\r\n", macAddress[0],
               macAddress[1], macAddress[2], macAddress[3], macAddress[4],
               macAddress[5]);
    UART_PRINT("\n\r\t============================================\n\r");

    return(ret);
}

int WifiInit(){

    int32_t ret;
    SlWlanSecParams_t security_params;
    pthread_t spawn_thread = (pthread_t) NULL;
    pthread_attr_t pattrs_spawn;
    struct sched_param pri_param;

    pthread_attr_init(&pattrs_spawn);
    pri_param.sched_priority = SPAWN_TASK_PRIORITY;
    ret = pthread_attr_setschedparam(&pattrs_spawn, &pri_param);
    ret |= pthread_attr_setstacksize(&pattrs_spawn, SL_TASKSTACKSIZE);
    ret |= pthread_attr_setdetachstate(&pattrs_spawn, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&spawn_thread, &pattrs_spawn, sl_Task, NULL);
    if(ret != 0){
        handleFatalError(SL_TASK_NOT_CREATED);
    }

    Network_IF_ResetMCUStateMachine();

    Network_IF_DeInitDriver();

    ret = Network_IF_InitDriver(ROLE_STA);
    if(ret < 0){
        handleFatalError(SIMPLELINK_DEVICE_NOT_STARTED);
    }

    DisplayAppBanner(APPLICATION_NAME, APPLICATION_VERSION);

    SetClientIdNamefromMacAddress();

    security_params.Key = (signed char*)SECURITY_KEY;
    security_params.KeyLen = strlen(SECURITY_KEY);
    security_params.Type = SECURITY_TYPE;

    ret = Network_IF_ConnectAP(SSID_NAME, security_params);
    if(ret < 0){
        handleFatalError(FAILED_TO_CONNECT_AP);
    }
    else{

        SlWlanSecParams_t securityParams;

        securityParams.Type = SECURITY_TYPE;
        securityParams.Key = (signed char*)SECURITY_KEY;
        securityParams.KeyLen = strlen((const char *)securityParams.Key);

        ret = sl_WlanProfileAdd((signed char*)SSID_NAME, strlen(SSID_NAME), 0, &securityParams, NULL, 7, 0);
        if(ret < 0){
            handleFatalError(FAILED_TO_ADD_AP_PROFILE);
        }
        else{
            LOG_INFO("profile added %s\r\n", SSID_NAME);
        }
    }

    return ret;
}

void mainThread(void * args){
    dbgEvent(ENTER_MAIN_THREAD);
    int32_t ret;
    UART_Handle uartHandle;
    MQTTClient_Handle mqttClientHandle;

    pthread_t           timer70_thread, timer500_thread, sensor_thread, stat_thread,
                        task_one_thread, task_two_thread;
    pthread_attr_t      attrs;
    struct sched_param  priParam;
    int                 retc;

    uartHandle = InitTerm();
    UART_control(uartHandle, UART_CMD_RXDISABLE, NULL);

    SPI_init();
    ADC_init();
    Timer_init();

    ret = ti_net_SlNet_initConfig();
    if(0 != ret)
    {
        handleFatalError(SL_NET_INIT_FAILED);
    }

    // Don't need LEDs
//    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);

    createMqttPublishQueue();
    createSensorThreadQueue();
    createTaskOneQueue();
    createTaskTwoQueue();

    createStatisticsQueue();

    ret = WifiInit();
    if(ret < 0){
        handleFatalError(WIFI_INIT_FAILED);
    }

    ret = MQTT_IF_Init(mqttInitParams);
    if(ret < 0){
        handleFatalError(MQTT_INIT_FAILED);
    }

#ifdef MQTT_SECURE_CLIENT
    setTime();
#endif

    /*
     * In case a persistent session is being used, subscribe is called before connect so that the module
     * is aware of the topic callbacks the user is using. This is important because if the broker is holding
     * messages for the client, after CONNACK the client may receive the messages before the module is aware
     * of the topic callbacks. The user may still call subscribe after connect but have to be aware of this.
     */

//    ret = MQTT_IF_Subscribe(mqttClientHandle, "JasonBoard", MQTT_QOS_0, JasonCB);
//    ret |= MQTT_IF_Subscribe(mqttClientHandle, "TerryBoard", MQTT_QOS_0, TerryCB);
    ret = MQTT_IF_Subscribe(mqttClientHandle, "Chain1", MQTT_QOS_0, ChainCB);
    ret |= MQTT_IF_Subscribe(mqttClientHandle, "start", MQTT_QOS_0, ChainCB);
    if(ret < 0){
        handleFatalError(MQTT_SUBSCRIPTION_FAILED);
    }
    else{
        LOG_INFO("Subscribed to all topics successfully\r\n");
    }

    mqttClientHandle = MQTT_IF_Connect(mqttClientParams, mqttConnParams, MQTT_EventCallback);

    if(mqttClientHandle < 0){
        handleFatalError(MQTT_CONNECT_FAILED);
    }

    pthread_attr_init(&attrs);

    priParam.sched_priority = 1;
    retc = pthread_attr_setschedparam(&attrs, &priParam);

    retc |= pthread_attr_setstacksize(&attrs, THREADSTACKSIZE);
#if 0
    retc = pthread_create(&timer70_thread, &attrs, timer70Thread, NULL);
    if (retc != 0) {
            /* pthread_create() failed */
        handleFatalError(PTHREAD_NOT_CREATED);
    }

    retc = pthread_create(&timer500_thread, &attrs, timer500Thread, NULL);
    if (retc != 0) {
            /* pthread_create() failed */
        handleFatalError(PTHREAD_NOT_CREATED);
    }
#endif
    retc = pthread_create(&sensor_thread, &attrs, sensor_task, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        handleFatalError(PTHREAD_NOT_CREATED);
    }

    retc = pthread_create(&task_one_thread, &attrs, task_one, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        handleFatalError(PTHREAD_NOT_CREATED);
    }

    retc = pthread_create(&task_two_thread, &attrs, task_two, NULL);
    if (retc != 0) {
        handleFatalError(PTHREAD_NOT_CREATED);
    }
    retc = pthread_create(&stat_thread, &attrs, task_statistics, NULL);
    if (retc != 0) {
        /* pthread_create() failed */
        //
        handleFatalError(PTHREAD_NOT_CREATED);
    }


    static mqttPublishQueueMessage queueElement;

    dbgEvent(BEFORE_MAIN_LOOP);
    while(1){

        dbgEvent(BEFORE_RECEIVE_MQTT_PUBLISH_QUE);
        queueElement = receiveFromMqttPublishQueue();
        dbgEvent(AFTER_RECEIVE_MQTT_PUBLISH_QUE);

        if(queueElement.event == APP_MQTT_PUBLISH){
            dbgEvent(BEFORE_PUBLISH_TO_MQTT);

            if (queueElement.topic_type == TASK_ONE_TOPIC) {
                MQTT_IF_Publish(mqttClientHandle,
                                "JasonBoard",
                                queueElement.payload,
                                strlen(queueElement.payload),
                                MQTT_QOS_0);
            }
            else if (queueElement.topic_type == TASK_TWO_TOPIC) {
                MQTT_IF_Publish(mqttClientHandle,
                                "Chain2",
                                queueElement.payload,
                                strlen(queueElement.payload),
                                MQTT_QOS_0);
            }
            else if (queueElement.topic_type == STATUS_TOPIC) {
                MQTT_IF_Publish(mqttClientHandle,
                                "JasonStatus",
                                queueElement.payload,
                                strlen(queueElement.payload),
                                MQTT_QOS_0);
            }


#if 0
            if (queueElement.topic_type == TASK_ONE_TOPIC) {
                MQTT_IF_Publish(mqttClientHandle,
                                "AjayBoard",
                                queueElement.payload,
                                            strlen(queueElement.payload),
                                            MQTT_QOS_0);
            }
            else if (queueElement.topic_type == TASK_TWO_TOPIC) {
                MQTT_IF_Publish(mqttClientHandle,
                                "Chain4",
                                queueElement.payload,
                                strlen(queueElement.payload),
                                MQTT_QOS_0);
            }
            else if (queueElement.topic_type == STATUS_TOPIC) {
                MQTT_IF_Publish(mqttClientHandle,
                                "AjayStatus",
                                queueElement.payload,
                                strlen(queueElement.payload),
                                MQTT_QOS_0);
            }
#endif
            dbgEvent(AFTER_PUBLISH_TO_MQTT);
        }
    }
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************
