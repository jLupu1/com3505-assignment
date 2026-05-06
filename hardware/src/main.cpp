#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <WiFi.h>

// Update with your network credentials
const char *ssid = "ssid";
const char *password = "password";

WebSocketsClient webSocket;

#define TEMP_PIN A1
#define ADC_MAX 4095.0
#define VREF 3.3

const long delayTime = 2000; // delay time
unsigned long previousMillis = 0;

const long updateDelay = 200;
unsigned long previousUpdate = 0;

const int numLeds = 6;
// Pins LEDs are plugged in - GGYYRR
const int ledPins[numLeds] = {13, 12, 11, 10, 9, 6};

bool ledStates[numLeds] = {false, false, false, false, false, false};

String currentMode = "manual"; // Can be "manual", "chase", "blink", "rainbow", "fire"

// ------ LED pattern delay variables ------
const long chaseLedDelay = 100;
unsigned long previousLedMillis = 500;

// ------ LED pattern sequence variables ------
// Chase
int currentChaseLed = 0;
int chaseDirection = 1;

// blink
bool isBlinkOn = false;

// rainbow
int rainbowStep = 0;
int rainbowDelay = 150;

// fire
unsigned long fireLastUpdate = 0;
int fireDelay = 50;

// ------- Temperature variables and functions  -------
float temperatureC;
bool firstTempRead = false;
float firstTemp;

float readTemp()
{
    int adcValue = analogRead(TEMP_PIN);
    float voltage = adcValue * VREF / ADC_MAX;
    float tempC = (voltage - 0.5) * 100.0;

    return tempC;
}

void handleTemp()
{
    temperatureC = readTemp();
    if (!firstTempRead)
    {
        firstTempRead = true;
        firstTemp = temperatureC;
    }
    bool res = webSocket.sendTXT("{\"type\":\"temperature\",\"data\":" + String(temperatureC) + "}");
    Serial.println("Temperature sent: " + String(temperatureC) + "°C, success: " + String(res));
}

// ------ LED control functions ------
void lightsOff()
{
    for (int i = 0; i < numLeds; i++)
    {
        digitalWrite(ledPins[i], LOW);
        ledStates[i] = false;
    }
}

void blink()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousLedMillis >= chaseLedDelay)
    {
        previousLedMillis = currentMillis;

        isBlinkOn = !isBlinkOn;
        for (int i = 0; i < numLeds; i++)
        {
            digitalWrite(ledPins[i], isBlinkOn ? HIGH : LOW);
            ledStates[i] = isBlinkOn;
        }
    }
}

void chase()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousLedMillis >= chaseLedDelay)
    {
        previousLedMillis = currentMillis;

        digitalWrite(ledPins[currentChaseLed], LOW);
        ledStates[currentChaseLed] = false;

        currentChaseLed = currentChaseLed + chaseDirection;

        if (currentChaseLed >= numLeds - 1)
        {
            currentChaseLed = numLeds - 1;
            chaseDirection = -1;
        }
        else if (currentChaseLed <= 0)
        {
            currentChaseLed = 0;
            chaseDirection = 1;
        }

        digitalWrite(ledPins[currentChaseLed], HIGH);
        ledStates[currentChaseLed] = true;
    }
}

// 3 LEDs that turn on and move across the array
void rainbow()
{
    unsigned long currentRainbowMillis = millis();
    if (currentRainbowMillis - previousLedMillis >= rainbowDelay)
    {
        previousLedMillis = currentRainbowMillis;
        lightsOff();

        for (int i = 0; i < 3; i++)
        {
            int activeLed = (rainbowStep + i) % numLeds;
            digitalWrite(ledPins[activeLed], HIGH);
            ledStates[activeLed] = true;
        }

        rainbowStep = (rainbowStep + 1) % numLeds;
    }
}

void fire()
{
    unsigned long currentFireMillis = millis();
    if (currentFireMillis - fireLastUpdate >= fireDelay)
    {
        fireLastUpdate = currentFireMillis;

        // Green lights off
        digitalWrite(ledPins[0], LOW);
        digitalWrite(ledPins[1], LOW);

        // flicker red and yellow lights at random intervals
        for (int i = 2; i < numLeds; i++)
        {
            bool isOn = random(0, 2);
            digitalWrite(ledPins[i], isOn ? HIGH : LOW);
            ledStates[i] = isOn;
        }

        fireDelay = random(20, 80);
    }
}

int binaryCounter = 0;
unsigned long lastBinary = 0;
unsigned long binaryDelay = 500;
void binary()
{
    unsigned long currentMillis = millis();
    if (currentMillis > lastBinary + binaryDelay)
    {
        lastBinary = currentMillis;
        binaryCounter += 1;
        if (binaryCounter == 64)
            binaryCounter = 1;
        ledStates[0] = binaryCounter & 1;
        ledStates[1] = binaryCounter & 2;
        ledStates[2] = binaryCounter & 4;
        ledStates[3] = binaryCounter & 8;
        ledStates[4] = binaryCounter & 16;
        ledStates[5] = binaryCounter & 32;
    }

    for (int i = 0; i < numLeds; i++)
    {
        digitalWrite(ledPins[i], ledStates[i] ? HIGH : LOW);
    }
}

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))

void temperature()
{
    float diff = temperatureC - firstTemp;
    if (diff > 0)
    {
        int lights = (int)(diff);
        ledStates[0] = true;
        ledStates[1] = true;
        ledStates[2] = true;
        for (int i = 0; i < MIN(lights, 3); i++)
        {
            ledStates[3 + i] = true;
        }
    }
    else
    {
        int lights = (int)(diff * -1.0);
        for (int i = MIN(2, lights); i >= 0; i--)
        {
            ledStates[2 - i] = true;
        }
    }

    for (int i = 0; i < numLeds; i++)
    {
        digitalWrite(ledPins[i], ledStates[i] ? HIGH : LOW);
    }
}

void webSocketEventHandler(WStype_t type, uint8_t *payload, size_t length)
{
    switch (type)
    {
    case WStype_CONNECTED:
        break;

    case WStype_TEXT:
    {
        Serial.print("Received text: ");
        Serial.println((char *)payload);

        // get the JSON object
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, payload);

        if (error)
        {
            Serial.print("JSON failure: ");
            Serial.println(error.c_str());
            return;
        }

        const String cmdType = doc["type"];
        if (currentMode != "manual")
        {
            lightsOff();
        }

        if (cmdType == "toggle")
        {
            currentMode = "manual";
            int ledNum = doc["data"];
            int pinNum = ledPins[ledNum];

            ledStates[ledNum] = !ledStates[ledNum];
            digitalWrite(pinNum, ledStates[ledNum] ? HIGH : LOW);

            Serial.println("Manual Toggle LED " + String(ledNum));
        }
        else if (cmdType == "pattern")
        {
            String patternType = doc["data"];

            if (currentMode != patternType)
            {
                currentMode = patternType;
                lightsOff();
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
    Serial.println("Connected to WiFi. IP address: " + WiFi.localIP().toString());

    webSocket.begin("172.20.10.3", 6767, "/hardware"); // Update with server's IP and port that is set up in app.py
    webSocket.onEvent(webSocketEventHandler);

    analogReadResolution(12);
    analogSetPinAttenuation(TEMP_PIN, ADC_11db);

    // set the pin mode for the LED pin
    for (int i = 0; i < numLeds; i++)
    {
        pinMode(ledPins[i], OUTPUT);
        digitalWrite(ledPins[i], LOW);
    }
}

void loop()
{
    webSocket.loop();
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= delayTime)
    {
        previousMillis = currentMillis;
        handleTemp();
    }

    if (currentMode == "blink")
    {
        blink();
    }
    else if (currentMode == "chase")
    {
        chase();
    }
    else if (currentMode == "rainbow")
    {
        rainbow();
    }
    else if (currentMode == "fire")
    {
        fire();
    }
    else if (currentMode == "binary")
    {
        binary();
    }
    else if (currentMode == "temperature")
    {
        temperature();
    }

    if (currentMillis - previousUpdate >= updateDelay)
    {
        String message = "{\"type\":\"ledArray\",\"data\":[";
        for (size_t i = 0; i < numLeds; i++)
        {
            message += ledStates[i] ? "255" : "0";
            if (i < numLeds - 1)
            {
                message += ",";
            }
        }
        message += "]}";

        bool res = webSocket.sendTXT(message);
    }
}
