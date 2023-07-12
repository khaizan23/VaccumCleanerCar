#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "esp_camera.h"
#include <WiFi.h>
//=====================================================================
#include <Arduino.h>
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCDaUXKO1XSlB58S62Q7skMTK6K_Pwl84o"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://vacum-robot-default-rtdb.firebaseio.com/" //https://sensor-1920d-default-rtdb.firebaseio.com/

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig configs;
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_M5STACK_PSRAM


const char* ssid = "Service Provider";
const char* password = "@Matinik298-2021";

extern int leftmotor1 =  15; // Left Motor
extern int leftmotor2 = 14;
extern int rightmotor1 = 13; // Right Motor
extern int rightmotor2 = 12;
extern int vacuum    = 4;
extern String Camerafeed = "";
extern String direction = "stop";
void startCameraServer();

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  pinMode(leftmotor1, OUTPUT);
  pinMode(leftmotor2, OUTPUT);
  pinMode(rightmotor1, OUTPUT);
  pinMode(rightmotor2, OUTPUT);
  pinMode(vacuum, OUTPUT);
  //initialize
  digitalWrite(leftmotor1, LOW);
  digitalWrite(vacuum, LOW);
  digitalWrite(leftmotor2, LOW);
  digitalWrite(rightmotor1, LOW);
  digitalWrite(rightmotor2, LOW);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_CIF);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  //========================================================================
  /* Assign the api key (required) */
  configs.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  configs.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&configs, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", configs.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  configs.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&configs, &auth);
  Firebase.reconnectWiFi(true);
  //=====================================================================
  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Camerafeed = WiFi.localIP().toString();
  Serial.println("' to connect");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    count = map(analogRead(2), 0, 4098, 0, 100);
   // count=random(100);
    // Write an Int number on the database path test/int
    if (Firebase.RTDB.setInt(&fbdo, "vacumRobot/battery", count)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
    //count++;
    if (Firebase.RTDB.setString(&fbdo, "vacumRobot/direction", direction)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}
