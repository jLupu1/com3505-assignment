#include <Arduino.h>
#include <WiFi.h>

const char *ssid = "SSID";
const char *password = "PASSWORD";

void setup()
{
    WiFi.begin(ssid, password);
}

void loop() {}