#ifndef mqtt_h
#define mqtt_h

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>

extern float waterTemp;

boolean reconnect(PubSubClient client);
void mqttCallback(char* topic, byte* payload, unsigned int length);

#endif