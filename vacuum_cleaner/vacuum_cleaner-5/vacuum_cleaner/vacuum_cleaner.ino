#include <SoftwareSerial.h>
SoftwareSerial gsmSerial(13, A7);
// defining the pins
int range = 20;
int turnTime = 600;
const int trigPin1 = 5;
const int echoPin1 = 8;
const int trigPin2 = 4;
const int echoPin2 = 7;
const int trigPin3 = 6;
const int echoPin3 = 11;
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
  Serial.begin(9600);
  SendMessage();
}
void loop() {
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

  Serial.print("battery read :  ");
  Serial.println(map(analogRead(batteryPin), 0, 1024, 0, 100));
  
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

void SendMessage() {

  Serial.println("Setting the GSM in text mode");
  gsmSerial.println("AT+CMGF=1\r");
  delay(1000);
  Serial.println("Sending SMS to the desired phone number!");
  gsmSerial.println("AT+CMGS=\"+306973991989\"\r");
  // Replace x with mobile number
  delay(1000);

  gsmSerial.println(" motor has been booted");    // SMS Text
  delay(200);
  gsmSerial.println((char)26);               // ASCII code of CTRL+Z
  delay(1000);
  gsmSerial.println();
  Serial.println("sent the text booted from SIM800");    // SMS Text
  gsmSerial.println("AT");
  delay(1000);
  Serial.println("Connecting...");
  Serial.println("Connected!");
  Serial.println("Setting the GSM in text mode");
  gsmSerial.println("AT+CMGF=1\r");
  delay(1000);
  gsmSerial.println("AT+CMGF=1");  //Set SMS to Text Mode
  delay(1000);
  // gsmSerial.println("AT+CNMI=1,2,0,0,0");  //Procedure to handle newly arrived messages(command name in text: new message indications to TE)
  // delay(1000);  Serial.println("Fetching list of received unread SMSes!");
  //  gsmSerial.println("AT+CMGL=\"REC UNREAD\"\r");
  // delay(1000);
}
