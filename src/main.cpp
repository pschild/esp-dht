#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <WifiHandler.h>
#include <MqttHandler.h>
#include <OTAUpdateHandler.h>
#include <ESP8266HTTPClient.h>
#include <dht.h>

#ifndef WIFI_SSID
  #error "Missing WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
  #error "Missing WIFI_PASSWORD"
#endif

#ifndef VERSION
  #error "Missing VERSION"
#endif

const String CHIP_ID = String("ESP_") + String(ESP.getChipId());

// void ping();
void publishDhtValues();
void startDeepSleep();
void ledTurnOn();
void ledTurnOff();
void onFooBar(char* payload);
void onOtaUpdate(char* payload);
void onMqttConnected();
void onMqttMessage(char* topic, char* message);

WifiHandler wifiHandler(WIFI_SSID, WIFI_PASSWORD);
MqttHandler mqttHandler("192.168.178.28", CHIP_ID);
OTAUpdateHandler updateHandler("192.168.178.28:9042", VERSION);

// Ticker pingTimer(ping, 60 * 1000);
Ticker deepSleepTimer(startDeepSleep, 10 * 1000); // 10s

dht DHT;
#define DHT22_PIN 4 // ~D2 on NodeMCU / GPIO4 on ESP12F

void setup() {
  Serial.begin(9600);
  pinMode(DHT22_PIN, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  wifiHandler.connect();
  mqttHandler.setup();
  mqttHandler.setOnConnectedCallback(onMqttConnected);
  mqttHandler.setOnMessageCallback(onMqttMessage);
  // pingTimer.start();

  // start OTA update immediately
  updateHandler.startUpdate();
}

void loop() {
  mqttHandler.loop();
  updateHandler.loop();
  // pingTimer.update();
  deepSleepTimer.update();
}

void ledTurnOn() {
  digitalWrite(LED_BUILTIN, LOW);
}

void ledTurnOff() {
  digitalWrite(LED_BUILTIN, HIGH);
}

/*void ping() {
  const String channel = String("devices/") + CHIP_ID + String("/version");
  mqttHandler.publish(channel.c_str(), VERSION);
}*/

void publishDhtValues() {
  DHT.read22(DHT22_PIN);

  const String tempChannel = String("devices/") + CHIP_ID + String("/temp");
  mqttHandler.publish(tempChannel.c_str(), String(DHT.temperature).c_str());

  const String humChannel = String("devices/") + CHIP_ID + String("/hum");
  mqttHandler.publish(humChannel.c_str(), String(DHT.humidity).c_str());
}

void startDeepSleep() {
  ESP.deepSleep(5 * 60e6); // 5min
  delay(100);
}

void onFooBar(char* payload) {
  if (strcmp(payload, "on") == 0) {
    ledTurnOn();
  } else if (strcmp(payload, "off") == 0) {
    ledTurnOff();
  }
}

void onOtaUpdate(char* payload) {
  updateHandler.startUpdate();
}

void onMqttConnected() {
  mqttHandler.subscribe("foo/+/baz");
  mqttHandler.subscribe("otaUpdate/all");

  publishDhtValues();
  deepSleepTimer.start(); // start deep sleep timer to allow enough time for the dht values to be sent
}

void onMqttMessage(char* topic, char* message) {
  if (((std::string) topic).rfind("foo/", 0) == 0) {
    onFooBar(message);
  } else if (strcmp(topic, "otaUpdate/all") == 0) {
    onOtaUpdate(message);
  }
}