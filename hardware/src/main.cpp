#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

const char *ssid = "SSID";
const char *password = "PASSWORD";

WebSocketsClient webSocket;

#define TEMP_PIN A1
#define ADC_MAX 4095.0
#define VREF 3.3

const long delayTime = 2000; //delay time 
unsigned long previousMillis = 0;

// Currently used to toggle LED
bool led_status = false;

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

    case WStype_TEXT: {
        Serial.print("Received text: ");
        Serial.println((char *)payload);

        // get the JSON object
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error) {
            Serial.print("JSON failure: ");
            Serial.println(error.c_str());
            return;
        }

        // Getting the values from JSON
        const String cmd_type = doc["type"];

        
        //TODO implement pattern, and single LED toggle
        // type can be pattern or toggle
        if(cmd_type == "toggle"){
            Serial.println("turning on LED");
            if (led_status == false)
            {
                digitalWrite(5,HIGH);
                led_status = true;
            }else{
                digitalWrite(5,LOW);
                led_status = false;
            }
        }
        break;
    }
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
    // Test for LED toggle
    pinMode(5,OUTPUT);

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
