
#include <LayadCircuits_SalengGSM.h>
#include <SoftwareSerial.h>

SoftwareSerial gsmSerial(13,A2);
// if you are using Arduino Mega or if you plan to use Serial 0 (pins D0 and D1), use the next line instead
// #define mySerial Serial1 // define as Serial, Serial1, Serial2 or Serial3

LayadCircuits_SalengGSM salengGSM = LayadCircuits_SalengGSM(&gsmSerial);

void setup()
{
  salengGSM.begin(9600); // this is the default baud rate
  Serial.begin(9600);
  Serial.print(F("Preparing Saleng GSM Shield.Pls wait for 10 seconds..."));
  delay(10000); // allow 10 seconds for modem to boot up and register
  salengGSM.initSalengGSM();
  Serial.println(F("Done"));
  salengGSM.sendSMS("09953879266","Hi, this is a test SMS from the Layad Circuits' Saleng GSM Shield. Have a nice day!");
  Serial.println(F("An SMS has been sent out."));
  Serial.println(F("Send an SMS to the phone number of the SIM card and see the message on screen."));
}

void loop()
{
  salengGSM.smsMachine(); // we need to pass here as fast as we can. this allows for non-blocking SMS transmission
  if(salengGSM.isSMSavailable()) // we also need to pass here as frequent as possible to check for incoming messages
  {
     salengGSM.readSMS(); // updates the read flag
     Serial.print("Sender=");
     Serial.println(salengGSM.smsSender);
     Serial.print("Whole Message=");
     Serial.println(salengGSM.smsRxMsg); // if we receive an SMS, print the contents of the receive buffer
  }
}
