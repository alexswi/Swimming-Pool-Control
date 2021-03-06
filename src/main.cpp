#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Update.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "WebPages.h"
#include "wifiCredentials.h"
#include "mqtt.h"
#include "relay.h"

#define BUTTON_A_PIN 39
#define BUTTON_B_PIN 35
#define BUTTON_C_PIN 34

hw_timer_t *timer = NULL;
const int wdtTimeout = 5000;

bool pump1State = false;
bool pump2State = false;
bool blinkState = false;

WiFiMulti wifiMulti;
unsigned long wifi_WatchdogLastUpdate;
long lastMsg = 0;
char msg[50];
int value = 0;
WiFiClient espClient;
PubSubClient client(espClient);

IPAddress local_IP(10, 1, 1, 81);
IPAddress gateway(10, 1, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 1, 1, 1);
IPAddress secundaryDNS(10, 1, 1, 1);

#define TFT_CS 16
#define TFT_RST 9 // you can also connect this to the Arduino reset \
                  // in which case, set this #define pin to -1!
#define TFT_DC 17

// Option 1 (recommended): must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

#define TFT_SCLK 5  // set these to be whatever pins you like!
#define TFT_MOSI 23 // set these to be whatever pins you like!
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

float p = 3.1415926;
unsigned long lastUpdate;
long lastReconnectAttempt = 0;
float waterTemp = 0;

WebServer server(80);

void testdrawtext(char *text, uint16_t color)
{
  tft.setCursor(0, 0);
  tft.setTextColor(color);
  tft.setTextWrap(true);
  tft.print(text);
}

void IRAM_ATTR resetModule()
{
  ets_printf("reboot\n");
  esp_restart();
}

void blink()
{
  if (blinkState)
  {
    tft.fillCircle(6, 120, 4, ST7735_GREEN);
  }
  else
  {
    tft.fillCircle(6, 120, 4, ST7735_YELLOW);
  }
  blinkState = !blinkState;
}

void tftUpdateTemp()
{
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  tft.setTextSize(2);
  tft.setCursor(80, 2);
  tft.print(waterTemp);
  tft.setCursor(80, 20);
  tft.print(22.2);
  tft.setCursor(80, 38);
  tft.print(33.3);
  tft.setCursor(80, 58);
  if (pump1State)
  {
    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    tft.print("ON ");
  }
  else
  {
    tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
    tft.print("OFF");
  }
  tft.setCursor(120, 58);
  if (pump2State)
  {
    tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
    tft.print("ON ");
  }
  else
  {
    tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
    tft.print("OFF");
  }
}

void tftPrintLables()
{
  tft.setCursor(2, 2);
  tft.setTextColor(ST7735_GREEN, ST7735_BLACK);
  tft.setTextSize(2);
  tft.print("Water:");
  tft.setCursor(2, 20);
  tft.print("Solar:");
  tft.setCursor(2, 38);
  tft.print("Out:");
  tft.setCursor(2, 58);
  tft.print("Pump:");
}

void printMessage(char *text)
{
  tft.setTextWrap(false);
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(2, 2);
  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);
  tft.print(text);
}

void handleNotFound()
{
  if (server.method() == HTTP_OPTIONS)
  {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Max-Age", "10000");
    server.sendHeader("Access-Control-Allow-Methods", "PUT,POST,GET,OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "*");
    server.send(204);
  }
  else
  {
    server.send(404, "text/plain", "");
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.print("");
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  digitalWrite(RELAY1, HIGH);
  digitalWrite(RELAY2, HIGH);
  tft.initR(INITR_BLACKTAB); // initialize a ST7735S chip, black tab
  Serial.println("Initialized");
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(2, 2);
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    tft.println("STA Failed to configure");
  }
  tft.print("Wifi ");
  wifiMulti.addAP(ssid1, password1);
  wifiMulti.addAP(ssid2, password2);
  Serial.println("");
  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    tft.print(".");
  }
  wifi_WatchdogLastUpdate = millis();
  delay(500);
  randomSeed(micros());
  tft.println("");
  tft.print(WiFi.SSID());
  tft.print(" ");
  tft.println(WiFi.RSSI());
  tft.print("IP: ");
  tft.println(WiFi.localIP());
  delay(1500);
  lastUpdate = millis();
  delay(1500);
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", homeIndex);
  });

  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.on("/pump1On", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    PumpSet(1, true);
    server.send(200, "text/html", homeIndex);
  });

  server.on("/pump1Off", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    PumpSet(1, false);
    server.send(200, "text/html", homeIndex);
  });

  server.on("/pump2On", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    PumpSet(2, true);
    server.send(200, "text/html", homeIndex);
  });

  server.on("/pump2Off", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    PumpSet(2, false);
    server.send(200, "text/html", homeIndex);
  });

  server.on("/stat", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Headers", "Origin, X-Requested-With, Content-Type, Accept");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Connection", "close");
    statBuild();
    server.send(200, "text/html", json_data);
  });

  server.onNotFound(handleNotFound);

  server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart(); }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      } });
  server.begin();
  tft.fillScreen(ST7735_BLACK);
  tftPrintLables();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  timer = timerBegin(0, 80, true);                  //timer 0, div 80
  timerAttachInterrupt(timer, &resetModule, true);  //attach callback
  timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
  timerAlarmEnable(timer);                          //enable interrupt
  lastReconnectAttempt = 0;
  Serial.println("done");
  delay(200);
}

void loop()
{
  timerWrite(timer, 0); //reset timer (feed watchdog)
  if (millis() - lastUpdate > 500)
  {
    blink();
    lastUpdate = millis();
    tftUpdateTemp();
  }
  if (millis() - wifi_WatchdogLastUpdate > 600000)
  {
    if (wifiMulti.run() != WL_CONNECTED)
    {
      Serial.println("WiFi not connected!");
    }
    wifi_WatchdogLastUpdate = millis();
  }
  if (!client.connected())
  {
    long now = millis();
    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect(client))
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    // Client connected
    client.loop();
  }
  server.handleClient();
  delay(1);
}