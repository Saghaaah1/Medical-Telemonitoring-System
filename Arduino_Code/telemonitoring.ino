/*
  Medical Telemonitoring System Using Arduino UNO
  Authors: Sara El Bari, Niamatellah Lahkim, Morgane Carbillet
  University of Western Brittany, 2023–2024

  Measures:
   - Heart rate
   - Oxygen saturation (SpO₂)
   - Body temperature

  Displays results on TFT LCD, triggers a buzzer for abnormal values,
  and sends data to ThingSpeak via Ethernet Shield.
*/

// --- Libraries ---
#include <SPI.h>
#include <Ethernet.h>
#include <TFT.h>
#include "DFRobot_BloodOxygen_S.h"
#include <ThingSpeak.h>

// --- TFT Display Pins ---
#define CS   10
#define DC    9
#define RST   8

// Create TFT object
TFT TFTscreen = TFT(CS, DC, RST);

// --- I2C Configuration for MAX30102 Sensor ---
#define I2C_COMMUNICATION
#define I2C_ADDRESS 0x57
DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, I2C_ADDRESS);

// --- ThingSpeak Configuration ---
unsigned long channelID = 2402209;               // ThingSpeak channel ID
const char* writeAPIKey = "MESIRNOUTBL1XR69M";   // ThingSpeak  API Key

// --- Ethernet MAC Address ---
byte mac[] = { 0xAB, 0x61, 0x0A, 0xAE, 0xDF, 0x56 };
EthernetClient client;

// --- Buzzer Pin ---
#define BUZZER 6

void setup() {
  // --- TFT Initialization ---
  TFTscreen.begin();
  TFTscreen.background(255, 255, 255);
  TFTscreen.stroke(255, 0, 0);

  // --- Serial Monitor ---
  Serial.begin(115200);
  Serial.println("Initializing MAX30102 sensor...");

  // --- Sensor Initialization ---
  while (!MAX30102.begin()) {
    Serial.println("MAX30102 init fail!");
    delay(1000);
  }

  Serial.println("MAX30102 initialized successfully.");
  Serial.println("Starting measurements...");
  MAX30102.sensorStartCollect();

  // --- Buzzer Setup ---
  pinMode(BUZZER, OUTPUT);
  noTone(BUZZER);

  // --- Ethernet & ThingSpeak ---
  Ethernet.begin(mac);
  ThingSpeak.begin(client);

  TFTscreen.background(255, 255, 255);
  TFTscreen.stroke(0, 0, 255);
  TFTscreen.text("System Ready...", 5, 10);
  delay(1500);
}

void loop() {
  // --- Clear Screen ---
  TFTscreen.background(255, 255, 255);

  // --- Read Sensor Data ---
  MAX30102.getHeartbeatSPO2();  // Updates internal structure with HR and SpO₂
  int spo2 = MAX30102.sHeartbeatSPO2.SPO2;
  int heartRate = MAX30102.sHeartbeatSPO2.Heartbeat;
  float temperature = MAX30102.getTemperature_C();

  // --- Print to Serial ---
  Serial.print("SpO2: "); Serial.print(spo2); Serial.println(" %");
  Serial.print("Heart Rate: "); Serial.print(heartRate); Serial.println(" bpm");
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.println("----------------------------");

  // --- Send Data to ThingSpeak ---
  ThingSpeak.setField(1, temperature);
  ThingSpeak.setField(2, heartRate);
  ThingSpeak.setField(3, spo2);

  int statusCode = ThingSpeak.writeFields(channelID, writeAPIKey);
  if (statusCode == 200) {
    Serial.println("ThingSpeak update successful.");
  } else {
    Serial.print("ThingSpeak update failed. HTTP error code: ");
    Serial.println(statusCode);
  }

  // --- Alert Conditions ---
  // Oxygen below 90%
  if (spo2 <= 90) {
    tone(BUZZER, 1000);
    delay(500);
    noTone(BUZZER);
  }
  // Heart rate out of range
  else if (heartRate < 60 || heartRate > 120) {
    tone(BUZZER, 1000);
    delay(500);
    noTone(BUZZER);
  }
  // Temperature abnormal
  else if (temperature < 30 || temperature > 35) {
    tone(BUZZER, 1000);
    delay(500);
    noTone(BUZZER);
  } else {
    noTone(BUZZER);
  }

  // --- Display Data on TFT ---
  TFTscreen.stroke(0, 0, 255);
  TFTscreen.setTextSize(1);
  TFTscreen.text(("SpO2: " + String(spo2) + " %").c_str(), 2, 20);
  TFTscreen.text(("Heart Rate: " + String(heartRate) + " bpm").c_str(), 2, 40);
  TFTscreen.text(("Temp: " + String(temperature) + " C").c_str(), 2, 60);

  // --- Delay 15 seconds before next update ---
  delay(15000);
}
