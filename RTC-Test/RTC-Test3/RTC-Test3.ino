#include <SoftwareSerial.h>
String str = "";

// Create software serial object to communicate with SIM800L
SoftwareSerial mySerial(13, A2); // SIM800L Tx & Rx is connected to Arduino #3 & #2
String data = "";
void setup()
{
  // Begin serial communication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);

  // Begin serial communication with Arduino and SIM800L
  mySerial.begin(9600);

  Serial.println("Initializing...");
  delay(1000);

  mySerial.println("AT"); // Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CSQ"); // Signal quality test, value range is 0-31 , 31 is the best
  updateSerial();
  mySerial.println("AT+CCID"); // Read SIM information to confirm whether the SIM is plugged
  updateSerial();
  mySerial.println("AT+CREG?"); // Check whether it has registered in the network
  updateSerial();
  data = "";
  mySerial.println("AT+CCLK?"); // Check whether it has registered in the network
  updateSerial();
  delay(1000);
  Serial.println("");
}

void loop()
{
  updateSerial2();
  Serial.print("time is: ");
  Serial.println(str);

  delay(5000);
}

void updateSerial()
{
  delay(500);
  while (Serial.available())
  {
    mySerial.write(Serial.read()); // Forward what Serial received to Software Serial Port
  }
  while (mySerial.available())
  {
    Serial.write(mySerial.read()); // Forward what Software Serial received to Serial Port
                                   // char rec = mySerial.read();
                                   // data += rec;
    // delay(50);
  }
}

void updateSerial2()
{
  delay(500);
  mySerial.println("AT+CCLK?"); // Check whether it has registered in the network
  if (mySerial.available())
  {
    Serial.write(mySerial.read()); // Forward what Software Serial received to Serial Port
    str = mySerial.readStringUntil('\n');
    // x = Serial.parseInt();
  }
}