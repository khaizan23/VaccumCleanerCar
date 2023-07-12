#include <LayadCircuits_SalengGSM.h>
#include <SoftwareSerial.h>

SoftwareSerial gsmSerial(13, A2);
LayadCircuits_SalengGSM salengGSM = LayadCircuits_SalengGSM(&gsmSerial);

// defining the pins
int range = 30;
int turnTime = 600;
const int trigPin1 = 5;
const int echoPin1 = 8;
const int trigPin2 = 4;
const int echoPin2 = 7;
const int trigPin3 = 3;
const int echoPin3 = 6;
int batteryPin = A0;
const int vacumPin = A1;
int in1 = 12;
int in2 = 11;
int in3 = 10;
int in4 = 9;

int irpin = 2;
// defining variables
long duration1;
long duration2;
long duration3;
int distanceleft;
int distancefront;
int distanceright;
int a = 0;
bool once = true;
int battery = 0;
int lowBattery = 20;
void setup() {
  pinMode(vacumPin,  OUTPUT);
  pinMode(trigPin1,  OUTPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(trigPin3, OUTPUT);// Sets the trigPin as an Output
  pinMode(echoPin1,  INPUT); // Sets the echoPin as an Input
  pinMode(echoPin2, INPUT);
  pinMode(echoPin3, INPUT);
  pinMode(irpin, INPUT);
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);
  salengGSM.begin(9600); // this is the default baud rate
  Serial.begin(9600);
  Serial.print(F("Preparing Saleng GSM Shield.Pls wait for 10 seconds..."));
  delay(10000); // allow 10 seconds for modem to boot up and register
  salengGSM.initSalengGSM();
  Serial.println(F("Done"));
  /*
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    delay(turnTime0);
     digitalWrite(in2, LOW);
    digitalWrite(in1, HIGH);
    digitalWrite(in4, LOW);
    digitalWrite(in3, HIGH);
    delay(turnTime0);*/
  digitalWrite(vacumPin, LOW);
  // Serial.begin(9600);
  salengGSM.sendSMS("09953879266", "Hi, THE ROBOT HAS BEEN BOOTED Have a nice day!");
  Serial.println(F("An SMS has been sent out for booted."));
}
void loop() {
  salengGSM.smsMachine();
  digitalWrite(trigPin1, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin1, LOW);
  duration1 = pulseIn(echoPin1,  HIGH);
  distanceleft = duration1 * 0.034 / 2;

  Serial.print("Distance1: ");
  Serial.println(distanceleft);
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin2, LOW);
  duration2 = pulseIn(echoPin2, HIGH);
  distancefront = duration2 * 0.034 / 2;
  Serial.print("Distance2: ");
  Serial.println(distancefront);
  digitalWrite(trigPin3, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin3, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin3, LOW);
  duration3 = pulseIn(echoPin3, HIGH);
  distanceright = duration3 * 0.034 / 2;
  Serial.print("Distance3: ");
  Serial.println(distanceright);
  battery = map(analogRead(batteryPin), 0, 1024, 0, 100);
  Serial.print("battery read :  ");
  Serial.println(battery);

  if (battery < lowBattery) {
    if (once) {
      salengGSM.sendSMS("09953879266", "Hi, THE ROBOT HAS a low battery please charge the battery");
      Serial.println(F("An SMS has been sent out for low batter."));
    }
    once = false;
  } else once = true; // only send mesage once battery is low to avoid multiple messages
  int s = digitalRead(irpin);
  if (s == HIGH)
  {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    delay(1000);
    a = 1;
  }
  if ((a == 0) && (s == LOW) && (distanceleft <= range && distancefront > range && distanceright <= range) ||
      (a == 0) && (s == LOW) && (distanceleft > range && distancefront > range && distanceright > range))
  {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    delay(turnTime);
  }
  if ((a == 1) && (s == LOW)
      || (s == LOW) && (distanceleft <= range && distancefront <= range && distanceright > range)
      || (s == LOW) && (distanceleft <= range && distancefront <= range && distanceright > range)
      || (s == LOW) && (distanceleft <= range && distancefront > range && distanceright > range) ||
      (distanceleft <= range && distancefront > range && distanceright > range)) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    delay(turnTime);
    a = 0;
  }
  if ((s == LOW) && (distanceleft > range && distancefront <= range && distanceright <= range)
      || (s == LOW) && (distanceleft > range && distancefront > range && distanceright <= range)
      || (s == LOW) && (distanceleft > range && distancefront <= range && distanceright > range) ) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
    delay(turnTime);
  }
}
