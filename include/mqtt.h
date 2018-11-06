#ifndef mqtt_h
#define mqtt_h

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

#define JSON_MAX 256

extern float waterTemp;
extern char json_data[];  

boolean reconnect(PubSubClient client);
void mqttCallback(char* topic, byte* payload, unsigned int length);
void statBuild();

#endif