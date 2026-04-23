#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

const char *ssid = "ssid";
const char *password = "password";

WebSocketsClient webSocket;

#define TEMP_PIN A1
#define ADC_MAX 4095.0
#define VREF 3.3

const long delayTime = 2000; //delay time 
unsigned long previousMillis = 0;

const int numLeds = 6;
const int ledPins[numLeds] = {13,12,11,10,9,6};

bool ledStates[numLeds] = {false, false, false, false, false, false};

String currentMode = "manual"; // Can be "manual", "chase", "blink", etc.

// ------ LED pattern delay variables ------
const long chaseLedDelay = 100; 
unsigned long previousLedMillis = 500;

// ------ LED pattern sequence variables ------
//Chase
int currentChaseLed = 0;  
int chaseDirection = 1;

// blink
bool isBlinkOn = false;



// ------- Temperature variables and functions  -------
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


// ------ LED control functions ------
void lights_off(){
    for(int i = 0; i < numLeds; i++){
        digitalWrite(ledPins[i], LOW);
        ledStates[i] = false;
    }
}

void blink() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - previousLedMillis >= chaseLedDelay) {
        previousLedMillis = currentMillis;

        isBlinkOn = !isBlinkOn;
        for (int i = 0; i < numLeds; i++)
        {
            digitalWrite(ledPins[i], isBlinkOn ? HIGH : LOW);
            ledStates[i] = isBlinkOn;
        }
    }
}

void chase() {
    unsigned long currentMillis = millis();
    
    if (currentMillis - previousLedMillis >= chaseLedDelay) {
        previousLedMillis = currentMillis;


        digitalWrite(ledPins[currentChaseLed], LOW);
        ledStates[currentChaseLed] = false;


        currentChaseLed = currentChaseLed + chaseDirection;


        if (currentChaseLed >= numLeds - 1) {
            currentChaseLed = numLeds - 1; 
            chaseDirection = -1;
        } else if (currentChaseLed <= 0) {
            currentChaseLed = 0;
            chaseDirection = 1;
        }


        digitalWrite(ledPins[currentChaseLed], HIGH);
        ledStates[currentChaseLed] = true;
    }
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

        const String cmdType = doc["type"];
        if (currentMode != "manual")
        {
            lights_off();
        }
        



        if(cmdType == "toggle"){
            currentMode = "manual";
            int ledNum = doc["data"]; 
            int pinNum = ledPins[ledNum]; 
            
            ledStates[ledNum] = !ledStates[ledNum]; 
            digitalWrite(pinNum, ledStates[ledNum] ? HIGH : LOW);
            
            Serial.println("Manual Toggle LED " + String(ledNum));
        }
        else if (cmdType == "pattern"){
            String patternType = doc["data"]; 
            
            if (currentMode != patternType) {
                currentMode = patternType; 
                lights_off();                        
                Serial.println("Switched to pattern: " + currentMode);
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

    //set the pin mode for the LED pin
    for(int i = 0; i < numLeds; i++){
        pinMode(ledPins[i],OUTPUT);
        digitalWrite(ledPins[i],LOW);
    }
}

void loop()
{
    webSocket.loop();
    unsigned long currentMillis = millis();
    if(currentMillis - previousMillis >= delayTime) {
        previousMillis = currentMillis;
        handleTemp();
    }

    if (currentMode == "blink") {
        blink();
    }
    else if (currentMode == "chase") {
        chase();
    }

}
