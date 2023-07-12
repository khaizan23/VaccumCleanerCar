// defining the pins
int range = 30;
int turnTime = 600;
const int trigPin1 = 5;
const int echoPin1 = 8;
const int trigPin2 = 4;
const int echoPin2 = 7;
const int trigPin3 = 3;
const int echoPin3 = 6;
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
