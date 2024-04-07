// MQTT
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <stdlib.h>
#include "MQTTAsync.h"
#include <unistd.h>
#define QOS 1
#define TIMEOUT 10000L

#define MAYOR 1
#define MINOR 0
#define PATCH 1
// Nextion
#include "Nextion_driver/nextion_driver.h"

// Nextion
Nextion_driver display = Nextion_driver("/dev/ttyTHS1", 115200);
void callbackFunction(char *msg)
{
    std::cout << "MSG: " << msg << std::endl;
}

// MQTT
const char SERVER_ADDRESS[] = {"mqtt://localhost:1883"};
const char CLIENT_ID[] = {"paho_cpp_async_consume"};
const char TOPIC[] = {"Tanque1/canal/#"};
int disc_finished = 0;
int subscribed = 0;
int finished = 0;

void onConnect(void *context, MQTTAsync_successData *response);
void onConnectFailure(void *context, MQTTAsync_failureData *response);
void connlost(void *context, char *cause);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message);
void onDisconnectFailure(void *context, MQTTAsync_failureData *response);
void onDisconnect(void *context, MQTTAsync_successData *response);
void onSubscribe(void *context, MQTTAsync_successData *response);
void onSubscribeFailure(void *context, MQTTAsync_failureData *response);
void onConnectFailure(void *context, MQTTAsync_failureData *response);
void onConnect(void *context, MQTTAsync_successData *response);

int main()
{
    printf("Nextion display MQTT  %d.%d.%d\n", MAYOR,MINOR,PATCH);
    // Nextion thread and call back
    std::thread t(std::bind(&Nextion_driver::infiniteThread, &display, callbackFunction));
    // MQTT
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    int rc;
    int ch;

    if ((rc = MQTTAsync_create(&client, SERVER_ADDRESS, CLIENT_ID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto exit;
    }

    if ((rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    while (!subscribed && !finished)
#if defined(_WIN32)
        Sleep(100);
#else
        usleep(10000L);
#endif

    if (finished)
        goto exit;

    do
    {
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');

    disc_opts.onSuccess = onDisconnect;
    disc_opts.onFailure = onDisconnectFailure;
    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start disconnect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }
    while (!disc_finished)
    {
#if defined(_WIN32)
        Sleep(100);
#else
        usleep(10000L);
#endif
    }

destroy_exit:
    MQTTAsync_destroy(&client);
exit:
    return rc;

    /*
        int inc = 0;
        while (inc <= 100)
        {
            // display.write_text("t0.txt=",inc);
            display.write_value("x0.val=", (inc * 100));
            long mapped = display.map(inc, 0, 1023, 0, 255);
            display.write_waveform(10, 0, (uint8_t)mapped);
            inc++;
            sleep(1);
        }
        */
    t.join(); // Wait for the thread to finish (which it won't, in this case)

    return 0;
}

void connlost(void *context, char *cause)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    printf("\nConnection lost\n");
    if (cause)
        printf("     cause: %s\n", cause);

    printf("Reconnecting\n");
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start connect, return code %d\n", rc);
        finished = 1;
    }
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char *)message->payload);
    if (strcmp("Tanque1/canal/temperature/sensor1", topicName) == 0)
    {
        display.write_value("x0.val=", (atof((char *)message->payload)) * 10);
        long mapped = display.map((atoi((char *)message->payload)), 0, 50, 0, 255);
        display.write_waveform(10, 0, (uint8_t)mapped);
    }
    if (strcmp("Tanque1/canal/turbidity/sensor1", topicName) == 0)
    {
        display.write_value("x1.val=", (atof((char *)message->payload)) * 10);
    }
    if (strcmp("Tanque1/canal/tds/sensor1", topicName) == 0)
    {
        display.write_value("x2.val=", (atof((char *)message->payload)) * 10);
    }
    if (strcmp("Tanque1/canal/level/sensor1", topicName) == 0)
    {
        display.write_value("x7.val=", (atof((char *)message->payload)) * 10);
    }

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);
    return 1;
}

void onDisconnectFailure(void *context, MQTTAsync_failureData *response)
{
    printf("Disconnect failed, rc %d\n", response->code);
    disc_finished = 1;
}

void onDisconnect(void *context, MQTTAsync_successData *response)
{
    printf("Successful disconnection\n");
    disc_finished = 1;
}

void onSubscribe(void *context, MQTTAsync_successData *response)
{
    printf("Subscribe succeeded\n");
    subscribed = 1;
}

void onSubscribeFailure(void *context, MQTTAsync_failureData *response)
{
    printf("Subscribe failed, rc %d\n", response->code);
    finished = 1;
}

void onConnectFailure(void *context, MQTTAsync_failureData *response)
{
    printf("Connect failed, rc %d\n", response->code);
    finished = 1;
}

void onConnect(void *context, MQTTAsync_successData *response)
{
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    int rc;

    printf("Successful connection\n");

    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n",
           TOPIC, CLIENT_ID, QOS);
    opts.onSuccess = onSubscribe;
    opts.onFailure = onSubscribeFailure;
    opts.context = client;
    if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Failed to start subscribe, return code %d\n", rc);
        finished = 1;
    }
}
