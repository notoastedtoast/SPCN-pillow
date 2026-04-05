#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "ScioSense_ENS160.h"
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

// ================= PINS =================
#define X_PIN A0
#define MIC_PIN A1
#define RELAY_PIN 7

// ================= AHT20 =================
Adafruit_AHTX0 aht;
float tempC = 0;
float humidity = 0;

// ================= ENS160 =================
ScioSense_ENS160 ens160(ENS160_I2CADDR_1);

// ================= MAX30100 =================
PulseOximeter pox;
uint32_t tsLastReport = 0;

// ================= TIMERS =================
uint32_t lastAHT = 0;
uint32_t lastENS = 0;
uint32_t lastAccel = 0;
uint32_t lastMic = 0;
uint32_t lastRelay = 0;

// ================= CALLBACK =================
void onBeatDetected() {
    Serial.println("Beat!");
}

// ================= SETUP =================
void setup() {
    Serial.begin(115200);
    delay(2000);

    Wire.begin(D2, D1);  // ESP8266

    pinMode(RELAY_PIN, OUTPUT);

    Serial.println("Starting system...");

    // AHT20
    if (aht.begin()) Serial.println("AHT20 OK");
    else Serial.println("AHT20 FAIL");

    // ENS160
    ens160.begin();
    if (ens160.available()) {
        Serial.println("ENS160 OK");
        ens160.setMode(ENS160_OPMODE_STD);
    } else Serial.println("ENS160 FAIL");

    // MAX30100
    if (!pox.begin()) {
        Serial.println("MAX30100 FAIL");
    } else {
        Serial.println("MAX30100 OK");
        pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
        pox.setOnBeatDetectedCallback(onBeatDetected);
    }

    Serial.println("Setup done.\n");
}

// ================= LOOP =================
void loop() {

    // ALWAYS update pulse sensor
    pox.update();

    // ---- Heart Rate ----
    if (millis() - tsLastReport > 1000) {
        Serial.print("HR: ");
        Serial.print(pox.getHeartRate());
        Serial.print(" bpm | SpO2: ");
        Serial.print(pox.getSpO2());
        Serial.println(" %");

        tsLastReport = millis();
    }

    // ---- AHT20 ----
    if (millis() - lastAHT > 2000) {
        sensors_event_t hum, temp;
        aht.getEvent(&hum, &temp);

        tempC = temp.temperature;
        humidity = hum.relative_humidity;

        Serial.print("Temp: ");
        Serial.print(tempC);
        Serial.print(" C | Hum: ");
        Serial.println(humidity);

        lastAHT = millis();
    }

    // ---- ENS160 ----
    if (millis() - lastENS > 3000) {
        if (ens160.available()) {
            ens160.set_envdata(tempC, humidity);
            ens160.measure(true);

            Serial.print("AQI: ");
            Serial.print(ens160.getAQI());
            Serial.print(" | TVOC: ");
            Serial.print(ens160.getTVOC());
            Serial.print(" | eCO2: ");
            Serial.println(ens160.geteCO2());
        }
        lastENS = millis();
    }

    // ---- Accelerometer ----
    if (millis() - lastAccel > 500) {
        int x = analogRead(X_PIN);
        Serial.print("X-axis: ");
        Serial.println(x);
        lastAccel = millis();
    }

    // ---- Microphone ----
    if (millis() - lastMic > 100) {
        int audio = analogRead(MIC_PIN);
        Serial.print("Mic: ");
        Serial.println(audio);
        lastMic = millis();
    }

    // ---- Relay toggle ----
    if (millis() - lastRelay > 5000) {
        static bool relayState = false;
        relayState = !relayState;
        digitalWrite(RELAY_PIN, relayState);
        Serial.println(relayState ? "Relay ON" : "Relay OFF");

        lastRelay = millis();
    }
}