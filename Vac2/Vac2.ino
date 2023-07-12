
#include <LayadCircuits_SalengGSM.h>
#include <SoftwareSerial.h>

SoftwareSerial gsmSerial(13, A2);
LayadCircuits_SalengGSM salengGSM = LayadCircuits_SalengGSM(&gsmSerial);

#include <ControlMotor.h>//Use the motor control library.
// Set the pins to use on the Arduino board.
ControlMotor control(12,11,10,9,5,6); // right motor 1, right motor 2, left motor 1, left motor 2, right PWM, left PWM

#include <Ultrasonic.h>//Uses the HC-SR04 ultrasonic sensor library.
Ultrasonic sensor(4,7,30000); // (Trig pin, Echo pin, maximum distance unit is us) i.e. 30000us = about 5 meters

int batteryPin = A0;
const int vacumPin = A1;

int mesurement_speed = 5;//Adjust sensor measurement speed.
long int distance = 0; //Declare a variable to store the distance.
int random_value = 0;//Stores a random value.

int a = 0;
bool once = true;
int battery = 0;
int lowBattery = 20;

void setup() 
{ 
  salengGSM.begin(9600); // this is the default baud rate
  Serial.begin(9600);
  Serial.print(F("Preparing Saleng GSM Shield.Pls wait for 10 seconds..."));
  delay(5000); // allow 10 seconds for modem to boot up and register
  salengGSM.initSalengGSM();
  Serial.println(F("Done"));

    digitalWrite(vacumPin, LOW);
  salengGSM.sendSMS("09121208354", "Hi, THE ROBOT HAS BEEN BOOTED Have a nice day!");
  Serial.println(F("An SMS has been sent out for booted."));
  delay(2000);
 
} 
 
void loop() 
{ 
  salengGSM.smsMachine();
  battery = map(analogRead(batteryPin), 0, 1024, 0, 100);
  Serial.print("battery read :  ");
  
  Serial.println(battery);
 if (battery < lowBattery) {
    if (once) {
      salengGSM.sendSMS("09121208354", "Hi, THE ROBOT HAS a low battery please charge the battery");
      salengGSM.smsMachine();
      Serial.println(F("An SMS has been sent out for low battery."));
    }
    once = false;
  } else once = true; //  avoid multiple messages
  
  control.Motor(150,1);//The car moves forward at a speed of 150.
  distance=sensor.Ranging(CM);//Measure the distance and store it in the distance variable.

  delay(mesurement_speed); //Delay to control sensor measurement speed.
 
 //The following applies to the case where there are no obstacles.
 Serial.print("No obstacle "); //Prints to the serial monitor that there are no obstacles.
 Serial.println(distance);//Print the distance.
 Serial.print("Random ");//Prints "Random".
 Serial.println(random_value);//Outputs a random value.

 random_value = random (2);//Create a random value to prevent the car from turning in one direction only.
  
  
  while(distance<30){//Applied when the distance from the obstacle is less than 30 cm.
        
  
  delay(mesurement_speed); //Delay to control sensor measurement speed.
  control.Motor(0,1);//Stop the motor.
  distance = sensor.Ranging(CM);
  delay(1000);

  if (random_value==0){// Corresponds when the random value is 0.
 Serial.print("Distance ");//Prints "Distance" on the serial monitor.
 Serial.println(distance);//Print the distance.
 Serial.print("Random ");//Prints "Random".
 Serial.println(random_value);//Outputs a random value.
 
  control.Motor(170,500);//The car turns right for 0.4 seconds.
  delay(400);}
  
  else if (random_value==1){//If the random value is 1.
 Serial.print("Distance ");//Prints "Distance" on the serial monitor.
 Serial.println(distance);//Print the distance.
 Serial.print("Random ");//Prints "Random".
 Serial.println(random_value);//Outputs a random value.
 
  control.Motor(170,-500);//Car turns left for 0.4 seconds.
  delay(400);}
  }
}