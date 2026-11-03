/*
  Conductivity code 2025-26 re-write (10/8/2026)
  Changes
    11/03/2026:
    - added lines to setup esp32 OTA wifi connection
    - switched ADC conversion factor to 3300/4095 (recommended by Claude)
    - changed serial.print to TelnetStream (so we can monitor on Terminal)
    - replaced delay statements with millis() if blocks to use internal hardware timer (WiFi requires non-blocking statements)
    - changed pin numbers (double check these with dev module)

    To do:
    - check adc conversion factor (would analogSetAttenuation(ADC_11db) be better?)
    - double check pin numbers + confirm specific pins work for our purposes
    - check over insertionSort and updateAnalog array functions (currently trashing lowest value to replace instead of oldest)
*/

#include <WiFi.h>
#include <ArduinoOTA.h>
#include <TelnetStream.h>
#include <DFRobot_ECPRO.h>

DFRobot_ECPRO ec;
DFRobot_ECPRO_PT1000 ecpt;

//pin definitions
#define EC_pin 1
#define TE_PIN 2
#define fan_pin 18     //stirring mechanism
#define switch_pin 4   //to turn on car
#define motor_pin_1 5  // previously motor_pin
#define motor_pin_2 6  //previously motor_unused_pin
#define MCU8 16
#define MCU7 17
#define linear_actuator_IN1 7
#define linear_actuator_IN2 15

// variables
uint16_t TE_Voltage, EC_Voltage;
float temp;
unsigned long lastReport = 0;

int switch_state;
unsigned long switchOnTime;
unsigned long currentTime;
unsigned long finalTime = 0;
int filterValue;
int initialValue;
int threshold = 5000;  //CHANGE, can NEVER exceed 3300 (cuz of the adc conversion factor)
float conductivity;
int diff = 0;         //store difference in analog value from initial value (filterValue - initialValue) to compare to threshold value
int analogArray[10];  //array to store analog readings (changed back to 10 from 5 to reduce fluctuation, experiment with this)
int n = 10;
bool firstRun = true;

void insertionSort(int arr[]) {  //sorts 10 value array read by probe
  for (int i = 1; i < n; i++) {
    int j = arr[i];

    while (i > 0 && arr[i - 1] > j) {
      arr[i] = arr[i - 1];
      i--;
    }

    arr[i] = j;
  }
}

void updateAnalogArray(int arr[]) {  //updates array with raw value read by probe
  for (int i = 0; i < n - 1; i++) {
    arr[i] = arr[i + 1];
  }

  arr[n - 1] = EC_Voltage;

  //median filter
  insertionSort(arr);
  filterValue = arr[n / 2];

  temp = ecpt.convVoltagetoTemperature_C((float)TE_Voltage / 1000);
  conductivity = ec.getEC_us_cm(EC_Voltage, temp);  //added temp variable to account for temp readings
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("car-ota", "password123", 6);  // channel 6, pick the quietest of 1/6/11


  ArduinoOTA.setHostname("esp32s3-car");
  ArduinoOTA.setPassword("ota1234");
  ArduinoOTA.begin();


  TelnetStream.begin();

  pinMode(linear_actuator_IN1, OUTPUT);
  pinMode(linear_actuator_IN2, OUTPUT);
  pinMode(fan_pin, OUTPUT);
  pinMode(motor_pin_1, OUTPUT);
  pinMode(motor_pin_2, OUTPUT);
  pinMode(switch_pin, INPUT_PULLUP);
  pinMode(MCU7, OUTPUT);
  pinMode(MCU8, OUTPUT);

  ec.setCalibration(1.372007);  // calibrated using DFRobot code on 10/6

  digitalWrite(fan_pin, HIGH);  // Fan always on
}



void loop() {
  ArduinoOTA.handle();

  EC_Voltage = (uint32_t)analogRead(EC_pin) * 3300 / 4095;  // fixed 10/6
  TE_Voltage = (uint32_t)analogRead(TE_PIN) * 3300 / 4095;
  //figure out analogSetAttenuation(ADC_11db) --> can help us scale esp32 adc

  updateAnalogArray(analogArray);  // analogRead(EC_pin) is in here as variable EC_Voltage

  switch_state = digitalRead(switch_pin);
  if (switch_state == 0) {           // switch ON
    if (firstRun) {

      for (int i = 0; i < n; i++) {
        analogArray[i] = (uint32_t)analogRead(EC_pin) * 3300 / 4095;
      }
      updateAnalogArray(analogArray);
      initialValue = filterValue;
      TelnetStream.print("Initial value: ");
      TelnetStream.println(initialValue);

      switchOnTime = millis();

      // retract linear actuator
      digitalWrite(MCU7, HIGH);
      digitalWrite(linear_actuator_IN1, LOW);
      digitalWrite(linear_actuator_IN2, HIGH);

      firstRun = false;
    }
    diff = filterValue - initialValue;
    currentTime = millis() - switchOnTime;

    if (diff <= threshold) {
      digitalWrite(motor_pin_1, HIGH);  // Run motor
      digitalWrite(motor_pin_2, LOW);
      digitalWrite(MCU8, HIGH);

      if (millis() - lastReport >= 2000) {

        TelnetStream.println("Threshold NOT passed.");
        TelnetStream.print(" Initial Value: ");
        TelnetStream.println(initialValue);
        TelnetStream.print("  Threshold:  ");
        TelnetStream.println(threshold);
        TelnetStream.print("  Voltage Difference: ");
        TelnetStream.println(diff);
        TelnetStream.print(" Analog Value: ");
        TelnetStream.println(EC_Voltage);
        TelnetStream.print(" Filter Value: ");
        TelnetStream.println(filterValue);
        TelnetStream.print(" Conductivity: ");
        TelnetStream.println(String(conductivity));
        TelnetStream.print("Temperature: ");
        TelnetStream.println(temp);
        TelnetStream.print("  Switch: ON  ");
        TelnetStream.print(" Time: ");
        TelnetStream.println(currentTime);
        lastReport = millis();
      }
    } else {
      digitalWrite(motor_pin_1, LOW);  // Stop motor
      digitalWrite(motor_pin_2, LOW);
      digitalWrite(MCU8, LOW);
      finalTime = currentTime;

      if (millis() - lastReport >= 2000) {
        TelnetStream.println("Threshold passed.");
        TelnetStream.print(" Initial Value: ");
        TelnetStream.println(initialValue);
        TelnetStream.print("  Threshold:  ");
        TelnetStream.println(threshold);
        TelnetStream.print("  Voltage Difference: ");
        TelnetStream.println(diff);
        TelnetStream.print(" Analog Value: ");
        TelnetStream.println(EC_Voltage);
        TelnetStream.print(" Filter Value: ");
        TelnetStream.println(filterValue);
        TelnetStream.print(" Conductivity: ");
        TelnetStream.println(String(conductivity));
        TelnetStream.print("Temperature: ");
        TelnetStream.println(temp);
        TelnetStream.print("  Switch: ON  ");
        TelnetStream.print("  Final Time: ");
        TelnetStream.println(finalTime);
        lastReport = millis();
      }
    }
  } else {  //if switch off
    digitalWrite(motor_pin_1, LOW);
    digitalWrite(motor_pin_2, LOW);
    digitalWrite(MCU8, LOW);

    if (millis() - lastReport >= 2000) {
      TelnetStream.println(" Switch: OFF ");
      TelnetStream.print(" Conductivity: ");
      TelnetStream.println(String(conductivity) + "us/cm\t");
      TelnetStream.print(" Analog Value: ");
      TelnetStream.println(EC_Voltage);
      lastReport = millis();
    }
    // Extend linear actuator
    digitalWrite(MCU7, HIGH);
    digitalWrite(linear_actuator_IN1, HIGH);
    digitalWrite(linear_actuator_IN2, LOW);

    initialValue = 0;
    diff = 0;
    firstRun = true;
  }
}