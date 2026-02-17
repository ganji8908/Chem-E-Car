/* EVERYTHING BUT THE SWITCH*/
/*LUMINOL IS JUST THE PUMP*/


#include <Adafruit_AS7341.h>

Adafruit_AS7341 as7341;

//#define switch_pin 9
#define motor_pin 3
#define unused_pin 4
#define pump_sleep 7     
//PUMP_SLEEP required to be driven HIGH to allow the motor driver for the car to work. 
#define car_sleep 8 

#define ASTEP_VAL 2999
#define ATIME_VAL 50 

//DO NOT USE A4(SDA), A5(SDL)  pins, THEY ARE LEFT FOR THE ADAFRUIT SENSOR ONLY. 

/* Refer to "Section 5: Export Variable Values to .csv File" for more information
Change "csvLogState" to true (1) or false (0) depending on whether or not you want to log "rawValue vs. time" data to a csv file
true (1) - enable csv logging
false (0) - disable csv logging, do regular printing instead
*/
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

unsigned int measured_intensity;
unsigned int threshold_intensity = 0; // Stopping team, change this depending on your experiment results; 
                                      //STEVEN : Changed this to 0 based of testing with the Rho Team. 
                                      //Car Motor cuts of when the threshold returns to 0.  

//STEVEN: Unsure why this delay is defined, not being used anywhere else in the code unsigned int motor_delay = 12000; // Stopping team, change this depending on when the car should be allowed to stop moving
 
int switch_state;
unsigned long switchOnTime;
unsigned long currentTime;
int flag = 1; // 1 = flip has been switched to ON, 0 = flip has been switched to OFF
bool stoppingFlag = 0;  // 1 = never rerun anything, 0 = reaction still going, continue operations
bool setupDelay=false;   //used to allow the car 8s only at the start of the set-up, and after initial setup, the loop of the car doesn't cause 8s delay to repeat.
// Added functionality to control the scale factor in setATIME



void setup()
{
  pinMode(pump_sleep, OUTPUT); 
  pinMode(car_sleep, OUTPUT); // STEVEN: Added to match switch car setup and wake up motor driver
   
  pinMode(motor_pin, OUTPUT);
  pinMode(unused_pin, OUTPUT);
  //pinMode(switch_pin, INPUT_PULLUP);
  Serial.begin(115200);

  if (!as7341.begin()) {
    Serial.println("Could not find AS7341");
    while (1) {
      delay(10);
    }
  }

  // Total integration time = (ATIME + 1) * (ASTEP + 1) * 2.78 µS
  // Set initial scale (would be overriden later)
  //STEVEN: USING HARDCODED VALUES FOR ATIME & ASTEP after testing with the Rho Team. 
  as7341.setATIME(ATIME_VAL); // integration time per step in increments of 2.78 us, range: 0 to 255
  as7341.setASTEP(ASTEP_VAL); // number of integration steps, range: 0 to 65535
  as7341.setGain(AS7341_GAIN_512X); // Gain can increase sensitivity, STEVEN: Left unchanged during testing by the Rho Team. 
  Serial.print("setup done");
}

void loop()
{
  // STEVEN: If stoppingFlag is true, we have hit the threshold and should not run the loop anymore.
  if (stoppingFlag) {
    return;
  }

  if (!as7341.readAllChannels()) {
    Serial.println("Error reading all channels!");
    return;
  }

  // Uncommenting the line below will tell you the integration time in milliseconds
  // regular_print(as7341.getTINT());
  Serial.println("STARTING main loop!");
  //STEVEN: CHANGED THE CHANNEL TO DETECT THE BLUE WAVELENGTH OF LIGHT, BASED OF THIS YEAR'S REACTION
  measured_intensity = as7341.getChannel(AS7341_CHANNEL_480nm_F3);
  //switch_state = digitalRead(switch_pin);

  /*
  Motor pin will go high if the switch is turned on for the linear actuator. To prevent the car motor from prematurely running,
  plug motor into battery only when you want to dispense liquid.
  */

//STEVEN: IMPORTANT NOTE: CAR STARTS MOVING THE MOMENT YOU TURN THE CAR ON. 


    //STEVEN: 8s THIS DELAY IS ADDED SO THAT WE CAN AVOID SENSING THE FIRST ZERO FROM THE STOPPING RHO REACTION, 
    //AND ONLY READ THE SECOND ZERO READING FROM THE REACTION.  
    //Car is only given 8s at the start for intial setup, after that, the 
    if(!setupDelay)
    {
      Serial.print("Starting new reaction protocol:\t"); Serial.print("MOTOR: ON\t"); Serial.println("Linear Actuator: Compressing...");
      // Start motor
      digitalWrite(pump_sleep, HIGH);   //STEVEN: set the sleep pin to high, to allow H-Bridge of the Motor driver to output higher voltage to the pump motor
      digitalWrite(car_sleep, HIGH);    //STEVEN: Wake up the motor driver
      digitalWrite(motor_pin, HIGH);  
      digitalWrite(unused_pin, LOW);

      Serial.println("IN the 8s delay time period....");
      unsigned long reactionStart=millis(); //holds the startTime of the reaction (specifically holds time since Arduino program started, but placing this in a variable helps us save the time.
      while (millis()-reactionStart<8000)
      {
        //execution gets stuck in this loop till 8000s
        //if(digitalRead(switch_pin)==1) {break;}  //Incase we decided to stop the car using the switch 
        Serial.print("Still in Setup Delay");
      }
      //We cannot use delay(8000), since this will stop the arduino and even the linear actuator would stop as well. 
      setupDelay=true; // set to true, so that this SETUP 8s is only given once.
      Serial.println("Exiting Setup Delay");
    }          

      /* We are using hardcoded values for the sensor so no need for Dynamic Calibration */

      // break state if switch is pulled mid-cycle

      as7341.readAllChannels();  // update channel states
      measured_intensity = as7341.getChannel(AS7341_CHANNEL_480nm_F3);  // fetch intensity value
      Serial.print("Measured Intensity:\t"); Serial.println(measured_intensity); 
      //Steven Added:
      Serial.print("Threshold Intensity:\t"); Serial.println(threshold_intensity); 
      //STEVEN: SWITCHED to == for this year's reaction

      if (measured_intensity==threshold_intensity) {
        Serial.print("Measured intensity= threshold, stop the Car motor");
        digitalWrite(pump_sleep, LOW); //cut off motor driver Power supply
        digitalWrite(car_sleep, LOW);  //Put driver to sleep
        digitalWrite(motor_pin, LOW); 
        stoppingFlag = true;           //STEVEN: Set flag to true so the car never restarts
        //waitForReswitch();  // hold infinite while loop until switch is repulled
        }   

}
