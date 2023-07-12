// defining the pins
const int trigPin1 = 3;
const int echoPin1 = 5;
const int trigPin2 = 6;
const int echoPin2 = 9;
const int trigPin3 = 10;
const int echoPin3 = 11;

int in1 = 4;
int in2 = 7;
int in3 = 8;
int in4 = 12;

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
}
void loop() {
  digitalWrite(trigPin1 , LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin1 , HIGH);
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
  if ((a == 0) && (s == LOW) && (distanceleft <= 15 && distancefront > 15 && distanceright <= 15) ||
      (a == 0) && (s == LOW) && (distanceleft > 15 && distancefront > 15 && distanceright > 15))
  {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  }
  if ((a == 1) && (s == LOW)
      || (s == LOW) && (distanceleft <= 15 && distancefront <= 15 && distanceright > 15)
      || (s == LOW) && (distanceleft <= 15 && distancefront <= 15 && distanceright > 15)
      || (s == LOW) && (distanceleft <= 15 && distancefront > 15 && distanceright > 15) ||
      (distanceleft <= 15 && distancefront > 15 && distanceright > 15)) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
    digitalWrite(in3, LOW);
    digitalWrite(in4, HIGH);
    delay(100);
    a = 0;
  }
  if ((s == LOW) && (distanceleft > 15 && distancefront <= 15 && distanceright <= 15)
      || (s == LOW) && (distanceleft > 15 && distancefront > 15 && distanceright <= 15)
      || (s == LOW) && (distanceleft > 15 && distancefront <= 15 && distanceright > 15) ) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
    digitalWrite(in3, HIGH);
    digitalWrite(in4, LOW);
  }
}
