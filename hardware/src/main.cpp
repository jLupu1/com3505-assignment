#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

const char *ssid = "SSID";
const char *password = "PASSWORD";

WebSocketsClient webSocket;

void webSocketEventHandler(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_CONNECTED:
        break;

    case WStype_TEXT:
        break;

    case WStype_DISCONNECTED:
        break;
    }
}

void setup()
{
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);

    webSocket.begin("192.168.1.100", 5000, "/hardware");
    webSocket.onEvent(webSocketEventHandler);
}

void loop()
{
    webSocket.loop();
}