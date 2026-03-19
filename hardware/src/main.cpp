#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

const char *ssid = "Jonathan iPhone";
const char *password = "cpfcilove6789";

WebSocketsClient webSocket;

#define TEMP_PIN A1
#define ADC_MAX 4095.0
#define VREF 3.3

const long delayTime = 2000; //delay time 
unsigned long previousMillis = 0;

float temperatureC;

float readTemp(){
    int adcValue = analogRead(TEMP_PIN);
    float voltage = adcValue * VREF / ADC_MAX;
    float tempC = (voltage - 0.5) * 100.0;

    return tempC;
}

void handleTemp(){
    temperatureC = readTemp();
    bool res = webSocket.sendTXT("{\"type\":\"temperature\",\"data\":" + String(temperatureC) + "}");
    Serial.println("Temperature sent: " + String(temperatureC) + "°C, success: " + String(res));
}


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
    {
        Serial.println("Connecting to WiFi...");
        delay(500);
    }
    Serial.println("Connected to WiFi");
    webSocket.begin("172.20.10.3", 6767, "/hardware");
    webSocket.onEvent(webSocketEventHandler);

    analogReadResolution(12);
    analogSetPinAttenuation(TEMP_PIN, ADC_11db);

}

void loop()
{
    webSocket.loop();
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= delayTime) {
        previousMillis = currentMillis;
        handleTemp();
    }

}