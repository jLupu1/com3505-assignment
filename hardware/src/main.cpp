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
const long ledDelay = 500; 
unsigned long previousLedMillis = 0;
int currentIndexChase = 0;

// ------ LED pattern sequence variables ------

// custom chase pattern sequence (index of pins)
const int chaseSequence[numLeds] = {0, 2, 4, 5, 3, 1}; 
int chaseStep = 0; // Tracks where in the sequence array


// ------- Temperature variable -------
float temperatureC;

float readTemp(){
    int adcValue = analogRead(TEMP_PIN);
    float voltage = adcValue * VREF / ADC_MAX;
    float tempC = (voltage - 0.5) * 100.0;

    return tempC;
}

void lights_off(){
    for(int i = 0; i < numLeds; i++){
        digitalWrite(ledPins[i], LOW);
        ledStates[i] = false;
    }
}

void blink() {
    unsigned long currentMillis = millis();
    

    if (currentMillis - previousLedMillis >= ledDelay) {
        previousLedMillis = currentMillis;


        int prevStep = (chaseStep == 0) ? (numLeds - 1) : (chaseStep - 1);
        int prevLedIndex = chaseSequence[prevStep];
        digitalWrite(ledPins[prevLedIndex], LOW);


        int currentLedIndex = chaseSequence[chaseStep];
        digitalWrite(ledPins[currentLedIndex], HIGH);


        chaseStep = (chaseStep + 1) % numLeds; 
    }
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

        const String cmdType = doc["type"];
        if (currentMode != "manual")
        {
            lights_off();
        }
        



        if(cmdType == "toggle"){
            currentMode = "manual"; // Switch to manual mode
            int ledNum = doc["data"]; 
            int pinNum = ledPins[ledNum]; 
            
            ledStates[ledNum] = !ledStates[ledNum]; // Flip the state
            digitalWrite(pinNum, ledStates[ledNum] ? HIGH : LOW);
            
            Serial.println("Manual Toggle LED " + String(ledNum));
        }
        else if (cmdType == "pattern"){
            String patternType = doc["data"]; 
            
            if (currentMode != patternType) {
                currentMode = patternType; 
                lights_off();              
                chaseStep = 0;             
                Serial.println("Switched to pattern: " + currentMode);
            }
        }

        // Getting the values from JSON
        // const String cmdType = doc["type"];


        
        // //TODO implement pattern, and single LED toggle
        // // type can be pattern or toggle
        // if(cmdType == "toggle"){
        //         int ledNum = doc["data"]; //index in array
        //         int pinNum = ledPins[ledNum]; //pin led is connected to
        //     if (ledStates[ledNum] == false)
        //     {
        //         digitalWrite(pinNum,HIGH);
        //         Serial.println("LED " + String(ledNum) + " turned on");
        //         Serial.println("Pin " + String(pinNum) + " set to HIGH");
        //         ledStates [ledNum] = true;
        //         Serial.println("LED states: " + String(ledStates[0]) + ", " + String(ledStates[1]) + ", " + String(ledStates[2]) + ", " + String(ledStates[3]) + ", " + String(ledStates[4]) + ", " + String(ledStates[5]));
        //     }else{
        //         digitalWrite(pinNum,LOW);
        //         Serial.println("LED " + String(ledNum) + " turned off");
        //         Serial.println("Pin " + String(pinNum) + " set to LOW");
        //         ledStates[ledNum] = false;
        //         Serial.println("LED states: " + String(ledStates[0]) + ", " + String(ledStates[1]) + ", " + String(ledStates[2]) + ", " + String(ledStates[3]) + ", " + String(ledStates[4]) + ", " + String(ledStates[5]));
        //     }
        // }
        
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

    // Serial.println("Current mode: " + currentMode);

    if (currentMode == "chase") {
        blink();
    }

}
