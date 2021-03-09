/*
 * mqttclient_if.c
 *
 *  Created on: Jan 22, 2020
 *      Author: a0227533
 */

#include <mqtt_if.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <stdio.h>

#include "debug_if.h"
//#include "mqtt_receive_queue.h"
#include "mqtt_publish_queue.h"
#include "mqtt_event_queue.h"
#include "task_one_queue.h"
#include "task_two_queue.h"
#include "debug.h"
#include "jsmn.h"
#include "str_converter.h"

enum{
    MQTT_STATE_IDLE,
    MQTT_STATE_INITIALIZED,
    MQTT_STATE_CONNECTED
};

#define MQTT_EVENT_RECV MQTT_EVENT_MAX  // event for when receiving data from subscribed topics

#define MQTTAPPTHREADSIZE   2048

// structure for linked list elements to manage the topics
struct Node
{
    MQTTClient_SubscribeParams topicParams;
    MQTT_IF_TopicCallback_f topicCB;
    struct Node* next;
};

#define MAXNODES    15
#define MAXTOPICLEN 20

static struct Node Nodes[MAXNODES];
static int nodesAllocated = 0;
static char topicList[MAXNODES][MAXTOPICLEN];
static int nextTopic;

struct Node* MyNodeAlloc() {
    if (nodesAllocated == MAXNODES) {
        handleFatalError(0x77); // Needs proper error name
    }
    else {
        nodesAllocated++;
    }
    return (&(Nodes[nodesAllocated - 1]));
}

static struct mqttContext
{
//    mqd_t msgQueue;
    pthread_mutex_t *moduleMutex;
    MQTTClient_Handle mqttClient;
    MQTT_IF_EventCallback_f eventCB;
    struct Node* head;
    int moduleState;
    uint8_t clientDisconnectFlag;
} mMQTTContext = {NULL, NULL, NULL, NULL, MQTT_STATE_IDLE, 0};

enum{
    APP_MQTT_PUBLISH,
    APP_MQTT_CON_TOGGLE,
    APP_MQTT_DEINIT,
    APP_BTN_HANDLER
};

static pthread_mutex_t mutex;


//https://github.com/zserge/jsmn/blob/master/example/simple.c
// strncmp but in JSON
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}


#define MAX_TOKENS  20
//static jsmntok_t tokens[MAX_TOKENS];
static jsmn_parser parser;
static jsmntok_t tokens[MAX_TOKENS];
// Callback invoked by the internal MQTT library to notify the application of MQTT events
void MQTTClientCallback(int32_t event, void *metaData, uint32_t metaDateLen, void *data, uint32_t dataLen)
{
    mqttEventQueueMessage message;
//    taskOneQueueMessage msgTaskOne;
    taskTwoQueueMessage msgTaskTwo;

    static int r, i;
    static int receivedSum;
    static int calculatedSum;
    // publicationCnt++; -> declared globally
    // -> needs to be sent to statistics thread

    /* Statistics Task */

    /*
     * statistics_thread.c
     *
     *
     * static int msgCount;
     *
     * void *task()
     *
     * msgCount = 0;
     *
     * while(1) {
     *
     *  receivedMsg = receiveMSGfromSTatsiQuee()
     *
     *  msgCount++;
     *
     *  receivedMsg.SensorCount/ChainCount -> 100th sensor message
     *  if (msgCount < receivedMsg.SensorCount) -> msgCount = 98, receivedMsg.SensorCount = 100
     *      lost_messages = SensorCount - msgCount
     * }
     *
     *
     *
     */

    switch((MQTTClient_EventCB)event)
    {

        case MQTTClient_OPERATION_CB_EVENT:
        {
            switch(((MQTTClient_OperationMetaDataCB *)metaData)->messageType)
            {
                case MQTTCLIENT_OPERATION_CONNACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: CONNACK\r\n");
                    message.event = MQTT_EVENT_CONNACK;
                    break;
                }
                case MQTTCLIENT_OPERATION_EVT_PUBACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: PUBACK\r\n");
                    message.event = MQTT_EVENT_PUBACK;
                    break;
                }

                case MQTTCLIENT_OPERATION_SUBACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: SUBACK\r\n");
                    message.event = MQTT_EVENT_SUBACK;
                    break;
                }

                case MQTTCLIENT_OPERATION_UNSUBACK:
                {
                    LOG_TRACE("MQTT CLIENT CB: UNSUBACK\r\n");
                    message.event = MQTT_EVENT_UNSUBACK;
                    break;
                }
            }
            break;
        }


        case MQTTClient_RECV_CB_EVENT:
        {
            LOG_TRACE("MQTT CLIENT CB: RECV CB\r\n");

            MQTTClient_RecvMetaDataCB *receivedMetaData;
//            char *topic;
//            char *payload;

            receivedMetaData = (MQTTClient_RecvMetaDataCB *)metaData;

            message.event = MQTT_EVENT_RECV;

            jsmn_init(&parser);
#if 0
            if (strncmp(receivedMetaData->topic, "JasonBoard", receivedMetaData->topLen) == 0)  ||
                    strncmp(receivedMetaData->topic, "TerryBoard", receivedMetaData->topLen) == 0)  {

                r = jsmn_parse(&parser, data, dataLen, tokens, MAX_TOKENS);
                if (r < 0) { handleFatalError(0x77); }

                for (i = 1; i < r; i++) {
                    if (jsoneq(data, &tokens[i], "SensorAvg") == 0) { // {"SensorReading": 211, "SensorCount": 5} // SensorAvg -> SensorAvg": # = SensorReading
                        if (strncmp(receivedMetaData->topic, "JasonBoard", receivedMetaData->topLen) == 0)
                            msgTaskOne.message_type = TIMER500_MSG_JASON;
                        else
                            msgTaskOne.message_type = TIMER500_MSG_TERRY;
                        msgTaskOne.SensorAvg = strToInt(data + tokens[i + 1].start,
                                                        tokens[i + 1].end - tokens[i + 1].start);

                    }
                    else if (jsoneq(data, &tokens[i], "SensorReading") == 0) { // {"SensorReading": 211, "SensorCount": 5} toekn[1] = "SensorReading" -> token[1] "SensorAvg": 3"
                        if (strncmp(receivedMetaData->topic, "JasonBoard", receivedMetaData->topLen) == 0)
                            msgTaskOne.message_type = TIMER70_MSG_JASON;
                        else
                            msgTaskOne.message_type = TIMER70_MSG_TERRY;
                        msgTaskOne.SensorReading = strToInt(data + tokens[i + 1].start,
                                                            tokens[i + 1].end - tokens[i + 1].start);
                    }
                    else if (jsoneq(data, &tokens[i], "SensorCount") == 0) {
                        msgTaskOne.SensorCount = strToInt(data + tokens[i + 1].start,
                                                          tokens[i + 1].end - tokens[i + 1].start);
                    }

                    else if (jsoneq(data, &tokens[i], "Time") == 0) {
                        msgTaskOne.Time = strToInt(data + tokens[i + 1].start,
                                                   tokens[i + 1].end - tokens[i + 1].start);
                    }

                    else if (jsoneq(data, &tokens[i], "Checksum") == 0) {
                        receivedSum = strToInt(data + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                        if (msgTaskOne.message_type == TIMER70_MSG_JASON || msgTaskOne.message_type == TIMER70_MSG_TERRY) {
                            calculatedSum = strToSum("SensorReading", strlen("SensorReading")) + msgTaskOne.SensorReading
                            + strToSum("SensorCount", strlen("SensorCount")) + msgTaskOne.SensorCount;
                        }
                        else if (msgTaskOne.message_type == TIMER500_MSG_JASON || msgTaskOne.message_type == TIMER500_MSG_TERRY) {
                            calculatedSum = strToSum("SensorAvg", strlen("SensorAvg")) + msgTaskOne.SensorAvg
                            + strToSum("Time", strlen("Time")) + msgTaskOne.Time;

                        }
                        else {
                            calculatedSum = 0;
                        }
                        if (receivedSum == calculatedSum) { // Message Verified
                            sendToTaskOneQueue(&msgTaskOne);

                        }
                    }
                    i++;
                }
            }
#endif
            if (strncmp(receivedMetaData->topic, "Chain1", receivedMetaData->topLen) == 0) {
                r = jsmn_parse(&parser, data, dataLen, tokens, MAX_TOKENS);
                if (r < 0) { handleFatalError(0x77); }
                for (i = 1; i < r; i++) {
                    if (jsoneq(data, &tokens[i], "Value") == 0) { // Value -> 509 in sum
                        msgTaskTwo.value = strToInt(data + tokens[i + 1].start,
                                                  tokens[i + 1].end - tokens[i + 1].start);

                    }
//                    else if (jsoneq(data, &tokens[i], "ChainCount") == 0) {
//                        msgTaskTwo.ChainCount = strToInt(data + tokens[i + 1].start,
//                                                    tokens[i + 1].end - tokens[i + 1].start);
//                    }
                    else if (jsoneq(data, &tokens[i], "Checksum") == 0) {
                        receivedSum = strToInt(data + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                        calculatedSum = strToSum("Value", strlen("Value")) + msgTaskTwo.value;
//                        calculatedSum += strToSum("ChainCount", strlen("ChainCount")) + msgTaskTwo.ChainCount;
                        if (receivedSum == calculatedSum) { // Message Verified
                            sendToTaskTwoQueue(&msgTaskTwo);
                        }
                    }
                }

            }
            break;
        }
        case MQTTClient_DISCONNECT_CB_EVENT:
        {
            LOG_TRACE("MQTT CLIENT CB: DISCONNECT\r\n");
            handleFatalError(0x77);
            break;
        }
//        break;
    }
    sendToMqttEventQueue(&message);
}

// Helper function used to compare topic strings and accounts for MQTT wild cards
int MQTTHelperTopicMatching(char *s, char *p)
{
    char *s_next = (char*)-1, *p_next;

    for(; s_next; s=s_next+1, p=p_next+1)
    {
        int len;

        if(s[0] == '#')
            return 1;

        s_next = strchr(s, '/');
        p_next = strchr(p, '/');

        len = ((s_next) ? (s_next - s) : (strlen(s))) + 1;
        if(s[0] != '+')
        {
            if(memcmp(s, p, len) != 0)
                return 0;
        }
    }
    return (p_next)?0:1;
}

// This is the application thread for the MQTT module that signals
void *MQTTAppThread(void *threadParams)
{
    int ret;
//    struct msgQueue queueElement;
    mqttEventQueueMessage queueElement;

    while(1) {
        queueElement = receiveFromMqttEventQueue();

        ret = 0;

        if(queueElement.event == MQTT_EVENT_CONNACK){
            LOG_TRACE("MQTT APP THREAD: CONNACK\r\n");

            pthread_mutex_lock(mMQTTContext.moduleMutex);

            // in case the user re-connects to a server this checks if there is a list of topics to
                // automatically subscribe to the topics again
            if(mMQTTContext.head != NULL){

                struct Node* curr = mMQTTContext.head;
                struct Node* prev;
                struct Node* tmp;

                // iterate through the linked list until the end is reached
                while(curr != NULL){

                    tmp = curr;

                    ret = MQTTClient_subscribe(mMQTTContext.mqttClient, &curr->topicParams, 1);
                    // if subscribing to a topic fails we must remove the node from the list since we are no longer subscribed
                    if(ret < 0){

                        LOG_ERROR("SUBSCRIBE FAILED: %s", curr->topicParams.topic);

                        // if the node to remove is the head update the head pointer
                        if(curr == mMQTTContext.head){
                            mMQTTContext.head = curr->next;
                            curr = curr->next;
                        }
                        else if(curr->next != NULL){
                            prev->next = curr->next;
                            curr = curr->next->next;
                        }
                        else{
                            prev->next = curr->next;
                            curr = curr->next;
                        }

                        // delete the node from the linked list
//                        free(tmp->topicParams.topic);
                    }
                    else{
                        prev = curr;
                        curr = curr->next;
                    }
                }
            }
            pthread_mutex_unlock(mMQTTContext.moduleMutex);
            // notify the user of the connection event
            mMQTTContext.eventCB(queueElement.event);
        }
    }
}

// this thread is for the internal MQTT library and is required for the library to function
void *MQTTContextThread(void *threadParams)
{
    int ret;

    LOG_TRACE("CONTEXT THREAD: RUNNING\r\n");

    MQTTClient_run((MQTTClient_Handle)threadParams);

    LOG_TRACE("CONTEXT THREAD: MQTTClient_run RETURN\r\n");

    pthread_mutex_lock(mMQTTContext.moduleMutex);

    ret = MQTTClient_delete(mMQTTContext.mqttClient);
    if(ret < 0){
        LOG_ERROR("client delete error %d", ret);
    }

    LOG_TRACE("CONTEXT THREAD: MQTT CLIENT DELETED")

    mMQTTContext.moduleState = MQTT_STATE_INITIALIZED;

    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    LOG_TRACE("CONTEXT THREAD EXIT\r\n");

    pthread_exit(0);

    return(NULL);
}

int MQTT_IF_Init(MQTT_IF_InitParams_t initParams)
{
    int ret;
    pthread_attr_t threadAttr;
    struct sched_param schedulingparams;
    pthread_t mqttAppThread = (pthread_t) NULL;

//    jsmn_init(&parser);

    if(mMQTTContext.moduleState != MQTT_STATE_IDLE){
        LOG_ERROR("library only supports 1 active init call\r\n");
        return -1;
    }

    // the mutex is to protect data in the mMQTTContext structure from being manipulated or accessed at the wrong time
//    mMQTTContext.moduleMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    mMQTTContext.moduleMutex = &mutex;
    if(mMQTTContext.moduleMutex == NULL){
        LOG_ERROR("malloc failed: mutex\r\n");
        return -1;
    }

    ret = pthread_mutex_init(mMQTTContext.moduleMutex, (const pthread_mutexattr_t *)NULL);
    if (ret != 0){
        LOG_ERROR("mutex init failed\r\n");
        return ret;
    }

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    // initializing the message queue for the MQTT module
    createMqttEventQueue();

    pthread_attr_init(&threadAttr);
    schedulingparams.sched_priority = initParams.threadPriority;
    ret = pthread_attr_setschedparam(&threadAttr, &schedulingparams);
    ret |= pthread_attr_setstacksize(&threadAttr, initParams.stackSize);
    ret |= pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    ret |= pthread_create(&mqttAppThread, &threadAttr, MQTTAppThread, NULL);
    if(ret == 0){
        mMQTTContext.moduleState = MQTT_STATE_INITIALIZED;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    return ret;
}

int MQTT_IF_Deinit(MQTTClient_Handle mqttClientHandle)
{
    handleFatalError(0x77); // Change the name later
#if 0
    int ret;
    struct msgQueue queueElement;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_INITIALIZED){
        if(mMQTTContext.moduleState == MQTT_STATE_CONNECTED){
            LOG_ERROR("call disconnect before calling deinit\r\n");
            pthread_mutex_unlock(mMQTTContext.moduleMutex);
            return -1;
        }
        else if(mMQTTContext.moduleState == MQTT_STATE_IDLE){
            LOG_ERROR("init has not been called\r\n");
            pthread_mutex_unlock(mMQTTContext.moduleMutex);
            return -1;
        }
    }

    queueElement.event = MQTT_EVENT_DESTROY;

    // since user called MQTT_IF_Deinit send the signal to the app thread so it may terminate itself
    ret = mq_send(mMQTTContext.msgQueue, (const char*)&queueElement, sizeof(struct msgQueue), 0);
    if(ret < 0){
        LOG_ERROR("msg queue send error %d", ret);
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    struct Node* tmp = mMQTTContext.head;

    // in case the user did not unsubscribe to topics this loop will free any memory that was allocated
    while(tmp != NULL){
        free(tmp->topicParams.topic);
        mMQTTContext.head = tmp->next;
        free(tmp);
        tmp = mMQTTContext.head;
    }

    // setting the MQTT module state back so that user can call init if they wish to use it again
    mMQTTContext.moduleState = MQTT_STATE_IDLE;
    pthread_mutex_unlock(mMQTTContext.moduleMutex);
#endif
    return 0;
}

MQTTClient_Handle MQTT_IF_Connect(MQTT_IF_ClientParams_t mqttClientParams, MQTTClient_ConnParams mqttConnParams, MQTT_IF_EventCallback_f mqttCB)
{
    int ret;
    pthread_attr_t threadAttr;
    struct sched_param priParam;
    pthread_t mqttContextThread = (pthread_t) NULL;
    MQTTClient_Params clientParams;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    // if the user has not called init this will return error since they're trying to connect without intializing the module
    if(mMQTTContext.moduleState != MQTT_STATE_INITIALIZED){

        if(mMQTTContext.moduleState == MQTT_STATE_CONNECTED){
            LOG_ERROR("mqtt library is already connected to a broker\r\n");
        }
        else{
            LOG_ERROR("must call init before calling connect\r\n");
        }
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return (MQTTClient_Handle)-1;
    }

    // copying the event callback the user registered for the module context
    mMQTTContext.eventCB = mqttCB;

    // preparing the data for MQTTClient_create
    clientParams.blockingSend = mqttClientParams.blockingSend;
    clientParams.clientId = mqttClientParams.clientID;
    clientParams.connParams = &mqttConnParams;
    clientParams.mqttMode31 = mqttClientParams.mqttMode31;

    mMQTTContext.mqttClient = MQTTClient_create(MQTTClientCallback, &clientParams);
    if(mMQTTContext.mqttClient == NULL){
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return (MQTTClient_Handle)-1;
    }

    pthread_attr_init(&threadAttr);
    priParam.sched_priority = 2;
    ret = pthread_attr_setschedparam(&threadAttr, &priParam);
    ret |= pthread_attr_setstacksize(&threadAttr, MQTTAPPTHREADSIZE);
    ret |= pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    ret |= pthread_create(&mqttContextThread, &threadAttr, MQTTContextThread, NULL);
    if(ret != 0){
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return (MQTTClient_Handle)-1;
    }

    // if the user included additional parameters for the client MQTTClient_set will be called
    if(mqttClientParams.willParams != NULL){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_WILL_PARAM, mqttClientParams.willParams, sizeof(MQTTClient_Will));
    }

    if(mqttClientParams.username != NULL){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_USER_NAME, mqttClientParams.username, strlen(mqttClientParams.username));
    }

    if(mqttClientParams.password != NULL){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_PASSWORD, mqttClientParams.password, strlen(mqttClientParams.password));
    }

    if(mqttClientParams.cleanConnect == false){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_CLEAN_CONNECT, &mqttClientParams.cleanConnect, sizeof(mqttClientParams.cleanConnect));
    }

    if(mqttClientParams.keepaliveTime > 0){
        MQTTClient_set(mMQTTContext.mqttClient, MQTTClient_KEEPALIVE_TIME, &mqttClientParams.keepaliveTime, sizeof(mqttClientParams.keepaliveTime));
    }

    ret = MQTTClient_connect(mMQTTContext.mqttClient);
    if(ret < 0){
        LOG_ERROR("connect failed: %d\r\n", ret);
    }
    else{
        mMQTTContext.moduleState = MQTT_STATE_CONNECTED;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    if(ret < 0){
        return (MQTTClient_Handle)ret;
    }
    else{
        return mMQTTContext.mqttClient;
    }
}

int MQTT_IF_Disconnect(MQTTClient_Handle mqttClientHandle)
{
    handleFatalError(0x77);
#if 0
    int ret;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_CONNECTED){
        LOG_ERROR("not connected to broker\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    mMQTTContext.clientDisconnectFlag = 1;

    ret = MQTTClient_disconnect(mqttClientHandle);
    if(ret < 0){
        LOG_ERROR("disconnect error %d", ret);
    }
    else{
        mMQTTContext.moduleState = MQTT_STATE_INITIALIZED;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    return ret;
#endif
    return 0;
}

int MQTT_IF_Subscribe(MQTTClient_Handle mqttClientHandle, char* topic, unsigned int qos, MQTT_IF_TopicCallback_f topicCB)
{
    int ret = 0;
    struct Node* topicEntry = NULL;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState == MQTT_STATE_IDLE){
        LOG_ERROR("user must call MQTT_IF_Init before using the MQTT module\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    // preparing the topic node to add it to the linked list that tracks all the subscribed topics
//    topicEntry = (struct Node*)malloc(sizeof(struct Node));
    topicEntry = MyNodeAlloc();
    if(topicEntry == NULL){
        LOG_ERROR("malloc failed: list entry\n\r");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

//    topicEntry->topicParams.topic = (char*)malloc((strlen(topic)+1)*sizeof(char));

    if (nextTopic >= MAXNODES){
        handleFatalError(0x77);
    }
    if (strlen(topic) + 1 >= MAXTOPICLEN) {
        handleFatalError(0x77);
    }
    topicEntry->topicParams.topic = topicList[nextTopic];
    nextTopic++;
    if(topicEntry->topicParams.topic == NULL){
        LOG_ERROR("malloc failed: topic\n\r");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    strcpy(topicEntry->topicParams.topic, topic);
    topicEntry->topicParams.qos = qos;
    topicEntry->topicCB = topicCB;

    // adding the topic node to the linked list
    topicEntry->next = mMQTTContext.head;
    mMQTTContext.head = topicEntry;

    if(mMQTTContext.moduleState == MQTT_STATE_CONNECTED){
        ret = MQTTClient_subscribe(mMQTTContext.mqttClient, &topicEntry->topicParams, 1);
        // if the client fails to subscribe to the node remove the topic from the list
        if(ret < 0){
            LOG_ERROR("subscribe failed %d. removing topic from list", ret);
//            free(topicEntry->topicParams.topic);
            handleFatalError(0x77); // Change the event name later

//            free(topicEntry);
        }
    }

    pthread_mutex_unlock(mMQTTContext.moduleMutex);
    return ret;
}

int MQTT_IF_Unsubscribe(MQTTClient_Handle mqttClientHandle, char* topic)
{
    handleFatalError(0x77);
#if 0
    int ret = 0;

    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_CONNECTED){
        LOG_ERROR("not connected to broker\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }

    MQTTClient_UnsubscribeParams unsubParams;
    unsubParams.topic = topic;

    ret = MQTTClient_unsubscribe(mMQTTContext.mqttClient, &unsubParams, 1);
    if(ret < 0){
        LOG_ERROR("unsub failed\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return ret;
    }
    else{
        // since unsubscribe succeeded the topic linked list must be updated to remove the topic node
        struct Node* curr = mMQTTContext.head;
        struct Node* prev;

        while(curr != NULL){

            // compare the current topic to the user passed topic
            ret = MQTTHelperTopicMatching(curr->topicParams.topic, topic);
            // if there is a match update the node pointers and remove the node
            if(ret == 1){

                if(curr == mMQTTContext.head){
                    mMQTTContext.head = curr->next;
                }
                else{
                    prev->next = curr->next;
                }
                free(curr->topicParams.topic);
                free(curr);
                pthread_mutex_unlock(mMQTTContext.moduleMutex);
                return ret;
            }
            prev = curr;
            curr = curr->next;
        }
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
    }

    LOG_ERROR("topic does not exist\r\n");
    pthread_mutex_unlock(mMQTTContext.moduleMutex);
#endif
    return -1;
}

int MQTT_IF_Publish(MQTTClient_Handle mqttClientParams, char* topic, char* payload, unsigned short payloadLen, int flags)
{
    pthread_mutex_lock(mMQTTContext.moduleMutex);
    if(mMQTTContext.moduleState != MQTT_STATE_CONNECTED){
        LOG_ERROR("not connected to broker\r\n");
        pthread_mutex_unlock(mMQTTContext.moduleMutex);
        return -1;
    }
    pthread_mutex_unlock(mMQTTContext.moduleMutex);

    return MQTTClient_publish(mqttClientParams,
                              (char*)topic,
                              strlen((char*)topic),
                              (char*)payload,
                              payloadLen,
                              flags);
}
