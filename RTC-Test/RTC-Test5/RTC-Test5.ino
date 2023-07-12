#include <SoftwareSerial.h>
String input_str, str, time_str, hour_str, year_str, month_str = "";
int year, day, month, hour, minute, second = 0;

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
  Serial.print("time now v4 is: ");
  Serial.println(input_str);
  // input_str = str; // "CCLK: 23/04/24,20:32:24+AT+CCLK?";
  //  input_str = str;
  //  Serial.println(input_str.length());
  // Extract the date and time string from the input string
  // Extract the date and time string from the input string
  // if (true)
  //{
  // if (input_str.length() > 20)
  // {
  String date_time_str = input_str.substring(6, 21);

  // Extract the individual date and time components from the date/time string
  String date_str = date_time_str.substring(0, 8);
  String time_str = date_time_str.substring(9);

  int year = date_str.substring(0, 2).toInt();
  int month = date_str.substring(3, 5).toInt();
  int day = date_str.substring(6).toInt();
  int hour = time_str.substring(0, 2).toInt();
  int minute = time_str.substring(3, 5).toInt();

  // Print the resulting values to the Serial Monitor
  Serial.print("Year: ");
  Serial.println(year);
  Serial.print("Month: ");
  Serial.println(month);
  Serial.print("Day: ");
  Serial.println(day);
  Serial.print("Hour: ");
  Serial.println(hour);
  Serial.print("Minute: ");
  Serial.println(minute);
  //}
  delay(2000);
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
  delay(500);
  if (mySerial.available())
  {
    Serial.write(mySerial.read()); // Forward what Software Serial received to Serial Port
    input_str = mySerial.readStringUntil('\n');
    // x = Serial.parseInt();
  }
}