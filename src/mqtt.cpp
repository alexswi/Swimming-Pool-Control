#include <ArduinoJson.h>
#include "mqtt.h"
#include "relay.h"

const char *stat_SwimmingPool_POWER = "stat/SwimmingPool/POWER";

boolean reconnect(PubSubClient client)
{
    Serial.print("Attempting MQTT connection...");
    if (client.connect("Swimming"))
    {
        Serial.println("connected");
        client.subscribe("tele/Mangueira/SENSOR");
        client.subscribe(stat_SwimmingPool_POWER);
    }
    return client.connected();
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived[");
    Serial.print(topic);
    Serial.print("]:");
    StaticJsonBuffer<500> JSONBuffer;
    if (String(topic) == stat_SwimmingPool_POWER)
    {
        JsonObject &power = JSONBuffer.parseObject(payload); //Parse message
        if (power.success())
        {
            int port = power["port"];
            int on = power["on"];
            Serial.print("port:");
            Serial.print(port);
            PumpSet(port, on == 1);
        }
        else
        {
            Serial.println("Parse power error");
            Serial.print("Message arrived [");
            Serial.print(topic);
            Serial.print("] ");
            for (int i = 0; i < length; i++)
            {
                Serial.print((char)payload[i]);
            }
            Serial.println();
        }
    }
    if (String(topic) == "tele/Mangueira/SENSOR")
    {
        JsonObject &parsed = JSONBuffer.parseObject(payload); //Parse message
        if (parsed.success())
        {
            const char *time = parsed["Time"]; // "2018-11-04T19:20:43"
            float temperature = parsed["DS18B20"]["Temperature"];
            Serial.print("Temperature:");
            Serial.println(temperature);
            waterTemp = temperature;
            Serial.print("time:");
            Serial.println(time);
        }
        else
        {
            Serial.println("Parse error");
            Serial.print("Message arrived [");
            Serial.print(topic);
            Serial.print("] ");
            for (int i = 0; i < length; i++)
            {
                Serial.print((char)payload[i]);
            }
            Serial.println();
        }
    }
}