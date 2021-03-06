// Example showing how to create an atuator for a servo.
// Connect red to +5V, Black or brown to GND and the last cable to Digital pin 3.
// The servo consumes much power and should probably have its own powersource.'
// The arduino might spontanally restart if too much power is used (happend
// to me when servo tried to pass the extreme positions = full load).
// Contribution by: Derek Macias


#include <Sensor.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <Servo.h> 
#include <RF24.h>

#define SERVO_DIGITAL_OUT_PIN 3
#define SERVO_MIN 0 // Fine tune your servos min. 0-180
#define SERVO_MAX 180  // Fine tune your servos max. 0-180
#define DETACH_DELAY 900 // Tune this to let your movement finish before detaching the servo
#define CHILD_ID 10   // Id of the sensor child

Sensor gw;
Servo myservo;  // create servo object to control a servo 
                // a maximum of eight servo objects can be created Sensor gw(9,10);
unsigned long timeOfLastChange = 0;
bool attachedServo = false;

            
void setup() 
{ 
  gw.begin();

  // Register all sensors to gw (they will be created as child devices)
  gw.sendSensorPresentation(CHILD_ID, S_COVER);

  // Fetch servo state at startup
  gw.getStatus(CHILD_ID, V_DIMMER);
  setRelayStatus(gw.getMessage()); // Wait here until status message arrive from gw
} 

void loop() 
{ 
   if (gw.messageAvailable()) {
      // New messsage from gw
      message_s message = gw.getMessage(); 
      setRelayStatus(message);
  }
  if (attachedServo && millis() - timeOfLastChange > DETACH_DELAY) {
     myservo.detach();
     attachedServo = false;
  }
} 

void setRelayStatus(message_s message) {
  myservo.attach(SERVO_DIGITAL_OUT_PIN);   
  attachedServo = true;
  if (message.header.type==V_DIMMER) { // This could be M_ACK_VARIABLE or M_SET_VARIABLE
     int val = atoi(message.data);
     myservo.write(SERVO_MAX + (SERVO_MIN-SERVO_MAX)/100 * val); // sets the servo position 0-180
     // Write some debug info
     Serial.print("Servo changed. new state: ");
     Serial.println(val);
   } else if (message.header.type==V_UP) {
     Serial.println("Servo UP command");
     myservo.write(SERVO_MIN);
     gw.sendVariable(CHILD_ID, V_DIMMER, 100);
   } else if (message.header.type==V_DOWN) {
     Serial.println("Servo DOWN command");
     myservo.write(SERVO_MAX); 
     gw.sendVariable(CHILD_ID, V_DIMMER, 0);
   } else if (message.header.type==V_STOP) {
     Serial.println("Servo STOP command");
     myservo.detach();
     attachedServo = false;

   }
   timeOfLastChange = millis();
}



