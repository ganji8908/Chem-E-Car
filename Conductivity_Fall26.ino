/*
  Conductivity code 2025-26 re-write (10/1/2026)
  Changes
    - added temperature readings for probe (using official website)
    - included temperature (temp) readings in conductivity calculation
    - added TE_PIN for temperature readings
*/

#include <DFRobot_ECPRO.h>
DFRobot_ECPRO ec;
DFRobot_ECPRO_PT1000 ecpt;

//pin definitions
#define EC_pin A1
#define TE_PIN A2
#define fan_pin A5 //stirring mechanism
#define switch_pin 9 //to turn on car
#define motor_pin_1 3 // previously motor_pin
#define motor_pin_2 4 //previously motor_unused_pin
#define MCU8 2
#define MCU7 7 
#define linear_actuator_IN1 5
#define linear_actuator_IN2 6

// variables
uint16_t rawValue, TE_Voltage;
float temp;
/////


int switch_state;
unsigned long switchOnTime;
unsigned long currentTime;
unsigned long finalTime = 0;
int filterValue;
int initialValue;
int threshold = 20; //determined by stopping team
float conductivity;
int diff = 0; //store difference in analog value from initial value (filterValue - initialValue) to compare to threshold value
int analogArray[5]; //array to store analog readings (changed from storing 10 to 5 values to speed up stopping process, continue experimenting with this)
int n = 5;
bool firstRun = true;

void insertionSort(int arr[]) { //sorts 5 value array read by probe
  for (int i = 1; i < n; i++) {
    int j = arr[i];

    while (i > 0 && arr[i - 1] > j) {
      arr[i] = arr[i-1];
      i--;
    }

    arr[i] = j;
  }
}

void updateAnalogArray(int arr[]){ //updates array with raw value read by probe
  for (int i = 0; i < n - 1; i++) {
    arr[i] = arr[i+1];   
  }

  arr[n-1] = rawValue;
  
  //median filter
  insertionSort(arr);
  filterValue = arr[n/2];

  temp = ecpt.convVoltagetoTemperature_C((float)TE_Voltage/1000);
  conductivity = ec.getEC_us_cm(rawValue, temp); //added temp variable to account for temp readings  
}

void setup() {
  pinMode(linear_actuator_IN1, OUTPUT);
  pinMode(linear_actuator_IN2, OUTPUT);
  pinMode(fan_pin, OUTPUT);
  pinMode(motor_pin_1, OUTPUT);
  pinMode(motor_pin_2, OUTPUT);
  pinMode(switch_pin, INPUT_PULLUP);
  pinMode(MCU7,OUTPUT);
  pinMode(MCU8,OUTPUT);

  Serial.begin(115200); //baud rate

  digitalWrite(fan_pin, HIGH); // Fan always on
}

void loop() {
  rawValue = (uint32_t)analogRead(EC_pin);
  TE_Voltage = (uint32_t)analogRead(TE_PIN) * 5000 / 1024;
  switch_state = digitalRead(switch_pin);
  if (switch_state == 0) { // switch ON
    updateAnalogArray(analogArray); // analogRead(EC_pin) is in here as variable rawValue
    diff = filterValue - initialValue; 
    if (firstRun)
    {
       // calibration
      ec.setCalibration(1.36);
      for(int i = 0; i < n; i++)
      {
        analogArray[i] = (uint32_t)analogRead(EC_pin);
      }
      updateAnalogArray(analogArray);
      initialValue = filterValue;
      Serial.print("Initial value: ");
      Serial.print(initialValue);

      switchOnTime = millis();

      // retract linear actuator
      digitalWrite(MCU7, HIGH);
      digitalWrite(linear_actuator_IN1, LOW);
      digitalWrite(linear_actuator_IN2, HIGH);

      firstRun = false;
    }

    currentTime = millis() - switchOnTime;

    if (diff <= threshold){
      digitalWrite(motor_pin_1, HIGH); // Run motor
      digitalWrite(motor_pin_2, LOW);
      digitalWrite(MCU8, HIGH);

      Serial.print("Threshold NOT passed.");
      Serial.print(" Initial Value: "); Serial.println(initialValue);
      Serial.print("  Threshold:  "); Serial.println(threshold);
      Serial.print("  Voltage Difference: "); Serial.println(diff); 
      Serial.print(" Analog Value: "); Serial.println(rawValue);
      Serial.print(" Filter Value: "); Serial.println(filterValue);
      Serial.print(" Conductivity: "); Serial.println(conductivity);
      Serial.print("Temperature: "); Serial.println(temp);
      Serial.print("  Switch: ON  ");
      Serial.print(" Time: "); Serial.println(currentTime);
      delay(100);
    }
    else {
      digitalWrite(motor_pin_1, LOW);  // Stop motor
      digitalWrite(motor_pin_2, LOW);
      digitalWrite(MCU8, LOW);
      finalTime = currentTime;

      Serial.print("Threshold passed.");
      Serial.print(" Initial Value: "); Serial.println(initialValue);
      Serial.print("  Threshold:  "); Serial.println(threshold);
      Serial.print("  Voltage Difference: "); Serial.println(diff); 
      Serial.print(" Analog Value: "); Serial.println(rawValue);
      Serial.print(" Filter Value: "); Serial.println(filterValue);
      Serial.print(" Conductivity: "); Serial.println(conductivity);
      Serial.print("Temperature: "); Serial.println(temp);
      Serial.print("  Switch: ON  ");
      Serial.print("  Final Time: "); Serial.println(finalTime);
    }
  }
  else{ //if switch off
    digitalWrite(motor_pin_1, LOW);
    digitalWrite(motor_pin_2, LOW);
    digitalWrite(MCU8, LOW);

    Serial.print(" Analog Value: "); Serial.print(rawValue);
    delay(100);
    Serial.println(" Switch: OFF ");

    // Extend linear actuator 
    digitalWrite(MCU7, HIGH);
    digitalWrite(linear_actuator_IN1, HIGH);
    digitalWrite(linear_actuator_IN2, LOW);

    initialValue = 0;
    diff = 0;
    firstRun = true;
  }
}
