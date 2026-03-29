#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include "ScioSense_ENS160.h"
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS 1000

// ================= AHT20 =================
Adafruit_AHTX0 aht;
float tempC = 0;
float humidity = 0;

// ================= ENS160 =================
ScioSense_ENS160 ens160(ENS160_I2CADDR_1); // 0x53

// ================= MAX30100 =================
PulseOximeter pox;
uint32_t tsLastReport = 0;

void onBeatDetected() {
    Serial.println("♥ Beat!");
}

// ================= TIMERS =================
uint32_t lastAHT = 0;
uint32_t lastENS = 0;

// ================= SETUP =================
void setup() {
    Serial.begin(115200);
    delay(2000);

    // 🔥 IMPORTANT for ESP8266
    Wire.begin(D2, D1);

    Serial.println("Starting system...");

    // ---- AHT20 ----
    Serial.print("AHT20: ");
    if (aht.begin()) {
        Serial.println("OK");
    } else {
        Serial.println("FAIL");
    }

    // ---- ENS160 ----
    Serial.print("ENS160: ");
    ens160.begin();
    if (ens160.available()) {
        Serial.println("OK");
        ens160.setMode(ENS160_OPMODE_STD);
    } else {
        Serial.println("FAIL");
    }

    // ---- MAX30100 ----
    Serial.print("MAX30100: ");
    if (!pox.begin()) {
        Serial.println("FAIL");
    } else {
        Serial.println("OK");
        pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
        pox.setOnBeatDetectedCallback(onBeatDetected);
    }

    Serial.println("Setup done.\n");
}

// ================= LOOP =================
void loop() {

    // Always keep MAX30100 running
    pox.update();

    // ---- Heart rate every 1s ----
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        Serial.print("HR: ");
        Serial.print(pox.getHeartRate());
        Serial.print(" bpm | SpO2: ");
        Serial.print(pox.getSpO2());
        Serial.println(" %");

        tsLastReport = millis();
    }

    // ---- AHT20 every 2s ----
    if (millis() - lastAHT > 2000) {
        sensors_event_t hum, temp;
        aht.getEvent(&hum, &temp);

        tempC = temp.temperature;
        humidity = hum.relative_humidity;

        Serial.print("Temp: ");
        Serial.print(tempC);
        Serial.print(" C | Humidity: ");
        Serial.println(humidity);

        lastAHT = millis();
    }

    // ---- ENS160 every 3s ----
    if (millis() - lastENS > 3000) {

        if (ens160.available()) {
            ens160.set_envdata(tempC, humidity);

            ens160.measure(true);
            ens160.measureRaw(true);

            Serial.print("AQI: ");
            Serial.print(ens160.getAQI());
            Serial.print(" | TVOC: ");
            Serial.print(ens160.getTVOC());
            Serial.print(" ppb | eCO2: ");
            Serial.println(ens160.geteCO2());
        }

        lastENS = millis();
    }
}