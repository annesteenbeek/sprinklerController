#include <ESP8266WiFi.h>
#include <MQTT.h>

#include "conf.h"

const int switchPin = 3;
const int flowPin = 2;
const int ledPin = 1;

WiFiClient net;
MQTTClient client;

bool sprinklerState = false;
volatile int pulseCount = 0;
long prevTime = millis();

void connect() {
    // wait for WiFi
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
    }

    // Connect to MQTT
    while (!client.connect(HOSTNAME)) {
        delay(1000);
    }

    client.subscribe(ROOT_TOPIC+"/setState");
    client.subscribe(ROOT_TOPIC+"/postFlow");
}

void messageReceived(String &topic, String &payload) {
    String subTopic = topic.substring(ROOT_TOPIC.length());
    if (subTopic == "/setState") {
        if (payload == "1") {
            digitalWrite(switchPin, HIGH);
            digitalWrite(ledPin, HIGH);
        } else {
            digitalWrite(switchPin, LOW);
            digitalWrite(ledPin, LOW);
        }
    } else if (subTopic == "/postFlow") {
        long curTime = millis();
        client.publish(ROOT_TOPIC+"/pulses", (String) pulseCount);
        client.publish(ROOT_TOPIC+"/tdiff", (String) (curTime-prevTime));
        pulseCount = 0;
        prevTime = curTime;
    }
}

void setup() {
    WiFi.hostname(HOSTNAME);
    WiFi.begin(WIFI_SSD, WIFI_PASS);
    pinMode(ledPin, OUTPUT);
    pinMode(switchPin, OUTPUT);
    pinMode(flowPin, INPUT);
    attachInterrupt(flowPin, pulseCounter, FALLING);

    client.begin(MQTT_BROKER, net);
    client.onMessage(messageReceived);

    connect();
}

void loop() {
    client.loop();
    delay(1000); // fixes some issues with WiFi stability

    // reconnect to MQTT if disconnected
    if (!client.connected()) {
        connect();
    }

    client.publish(ROOT_TOPIC + "/heartbeat", "1");
}


/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}