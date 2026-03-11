/*
Code last updated: 3/1/2026
  - updated calibration - threshold difference to be set by stopping team
      - compares filterValue - initialValue to threshold difference
      - now prints out difference for each reading
❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗
❗❗❗ NOTICE TO EVERYONE: BEFORE UPLOADING UPDATED CODE FILES TO THE GOOGLE DRIVE, UPDATE THE CHANGELOG HERE ❗❗❗
❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗❗
*/

#include <DFRobot_ECPRO.h>

//pin definitions
#define EC_pin A1
#define fan_pin A5
#define switch_pin 9 //to turn on car
#define motor_pin 3
#define linear_actuator_IN1 5
#define linear_actuator_IN2 6

//data in csv format
#define csvLogState 0
#if csvLogState
#define csv_print(...) Serial.print(__VA_ARGS__)
#define csv_println(...) Serial.println(__VA_ARGS__)
#define regular_print(...)
#define regular_println(...)
#else
#define csv_print(...)
#define csv_println(...)
#define regular_print(...) Serial.print(__VA_ARGS__)
#define regular_println(...) Serial.println(__VA_ARGS__)
#endif

//define variables
int switch_state;
unsigned long switchOnTime;
unsigned long currentTime;
unsigned long finalTime = 0;
int flag = 1; // 0 = switch ON, 1 = switch OFF
int rawValue;
int filterValue;
int initialValue = -1;
int threshold = 100; //change based on stopping team measured threshold difference
int val = 0;

bool calibrated = false;
int calibrationReadings[10];
int calibrationSum = 0;
int analogArray[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // n = 10
int n = 10;

void calibrateECScale() {  //probe
  Serial.println("Calibrating EC sensor...");

  calibrationSum = 0;
  for (int i = 0; i < 10; i++) {
    calibrationReadings[i] = analogRead(EC_pin);
    analogArray[i] = calibrationReadings[i];
    calibrationSum += calibrationReadings[i];
    delay(100);
  }

  initialValue = calibrationSum / 10;
  //threshold = initialValue - 250;

  Serial.print("Calibration complete. Initial value: ");
  Serial.print(initialValue);
  Serial.print(" | Threshold set to: ");
  Serial.println(threshold);

  calibrated = true;
}

// Sort the given array (source: geeksforgeeks.org)
void insertionSort(int arr[]) { // this is actually tempArr[]
  for (int i = 1; i < n; ++i) {
    int key = arr[i];
    int j = i - 1;

    while (j >= 0 && arr[j] > key) {
      arr[j+1] = arr[j];
      j = j - 1;
    }

    arr[j + 1] = key;
  }
}

int medianFilter(int tempArr[]) {
  int value = 0;

  insertionSort(tempArr);

  if (n % 2 == 0) { // even
    value = (tempArr[n/2 - 1] + tempArr[n/2]) / 2;
  }
  else { // odd
    value = tempArr[n/2];
  }

  return value;
}

void updateAnalogArray(int arr[]) {
  int tempArr[10];

  for (int i = 0; i < n - 1; i++) {
    arr[i] = arr[i+1];    //each value moves one space left
    tempArr[i] = arr[i];  //tempArr = arr
  }
  rawValue = (uint32_t)analogRead(EC_pin)* 5000/1024; //reads voltage from probe (adjusted to 5V range)
  arr[n-1] = rawValue; //last value = latest reading
  tempArr[n-1] = arr[n-1]; //tempArr = arr

  filterValue = medianFilter(tempArr); // update filterValue - filters out noise
  val = filterValue - initialValue;
}

void setup()
{
  //analogReference(EXTERNAL); //set Vref to 3.3V for ADC (connect wire from AREF pin to 3.3 V pin)
  pinMode(linear_actuator_IN1, OUTPUT);
  pinMode(linear_actuator_IN2, OUTPUT);
  pinMode(fan_pin, OUTPUT);
  pinMode(motor_pin, OUTPUT);
  pinMode(switch_pin, INPUT_PULLUP);
  Serial.begin(115200);

  digitalWrite(fan_pin, HIGH); // Fan always on
}

void loop()
{
  switch_state = digitalRead(switch_pin); // stores switch position
  updateAnalogArray(analogArray); // analogRead(EC_pin) is in here

  if (switch_state == 0) // Switch ON
  { 
    if (!calibrated) {
      calibrateECScale();  // one-time calibration to find initialValue
    }
    
    // Retract linear actuator 
    digitalWrite(linear_actuator_IN1, LOW);
    digitalWrite(linear_actuator_IN2, HIGH);

    if (flag == 1) { 
      switchOnTime = millis();
      flag = 0;
    }

    currentTime = millis() - switchOnTime;

      if (val >= threshold) {
        digitalWrite(motor_pin, LOW);  // Stop motor
        finalTime = currentTime;

        regular_println("Threshold passed.");
        regular_print(" Initial Value: "); regular_print(initialValue);
        regular_print(" Difference (filterValue - InitialValue): "); regular_print(val); //difference between filtered value and initial value (being compared to threshold difference)
        regular_print(" Threshold: "); regular_print(threshold);
        regular_print(" Analog Value: "); regular_print(rawValue);
        regular_print(" Filter Value: "); regular_print(filterValue);
        regular_print(" Final Time: "); regular_println(finalTime);

        csv_print(finalTime); csv_print(","); csv_print(rawValue); csv_print('\n');
      } else {
        digitalWrite(motor_pin, HIGH); // Run motor

        regular_print("Threshold NOT passed.");
        regular_print(" Initial Value: "); regular_print(initialValue);
        regular_print(" Threshold: "); regular_print(threshold);
        regular_print(" Analog Value: "); regular_print(rawValue);
        regular_print(" Filter Value: "); regular_print(filterValue);
        regular_print(" Difference: "); regular_print(val);        
        regular_print(" Switch: ON ");
        regular_print(" Time: "); regular_print(currentTime);
        regular_print('\n');

        csv_print(currentTime); csv_print(","); csv_print(filterValue); csv_print('\n');
      }
    }
  else if (switch_state == 1) // Switch OFF
  {
    digitalWrite(motor_pin, LOW);

    regular_print(" Analog Value: ");
    regular_print(filterValue);
    regular_println(" Switch: OFF ");

    // Extend linear actuator 
    digitalWrite(linear_actuator_IN1, HIGH);
    digitalWrite(linear_actuator_IN2, LOW);

    flag = 1;
    calibrated = false;
    finalPrintFlag = false;
    initialValue = -1;
    calibrationSum = 0;
    val = 0;
  }
}

