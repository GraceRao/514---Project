/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <stdlib.h>
#include <SwitecX25.h>
#include <Wire.h>

#define STEPS 945

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "7d416120-3b50-4969-bedf-f068bedb920b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

static BLECharacteristic *pCharacteristic;

// Define LED pins
const int ledPinClose = 6; // 当距离大于8且小于16时亮的LED灯的引脚
const int ledPinFar = 5; // 当距离大于等于16时亮的LED灯的引脚

// Define the photoresistor pin
const int photoResistorPin = A10; // 假设光敏电阻连接到A0引脚
int lightResistance; // 用于存储光照强度的变量
const char * characteristicInitValue = "Server started";

SwitecX25 motor(STEPS, D0, D1, D2, D3);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  // setup ble
  BLEDevice::init("Grace_Test");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue(characteristicInitValue);
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");

  // init LED pins
  pinMode(ledPinClose, OUTPUT);
  pinMode(ledPinFar, OUTPUT);

  // init the photoresistor pin as an input
  pinMode(photoResistorPin, INPUT);

  // init motor
  motor.zero(); 
}

void loop() {
  lightResistance = analogRead(photoResistorPin);
  int targetStep = 0;

  // read light sensor resistance
  if (lightResistance >= 2300) {
    const char * value = pCharacteristic->getValue().c_str();

    // if characteristic is initial value, do not check further
    if (strcmp(value, characteristicInitValue) == 0) {
      Serial.println("Device not connected");
    } else {
      char * result = (char *) value;
      char * token = strtok(result, " ");
      char * distance = strtok(NULL, " ");
      char * distanceLevel = strtok(NULL, " ");
      
      // set led
      if (strcmp(distanceLevel, "(Intermediate)") == 0) {
          digitalWrite(ledPinClose, HIGH);
          digitalWrite(ledPinFar, LOW);
      } else if (strcmp(distanceLevel, "(Far)") == 0) {
          digitalWrite(ledPinClose, LOW);
          digitalWrite(ledPinFar, HIGH);
      } else {
          digitalWrite(ledPinClose, LOW);
          digitalWrite(ledPinFar, LOW);
      }

      // set motor
      distance[strlen(distance) - 1] = '\0';
      double distanceDouble = atof(distance);
      targetStep = distanceDouble / 10.0 > 1 ? STEPS : (int)(distanceDouble / 10.0 * STEPS);
    }
  } else {
    digitalWrite(ledPinClose, LOW);
    digitalWrite(ledPinFar, LOW);
    targetStep = 0;
    Serial.printf("lightResistance is not in threshold: %d\n", lightResistance);
  }

  motor.setPosition(targetStep);
  while(motor.currentStep != motor.targetStep){
    motor.update();
  }

  Serial.println(pCharacteristic->getValue().c_str());
  delay(2000);
}