#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"
#include "esp_camera.h"
#include <WiFi.h>
//=====================================================================
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include <time.h>



// Camera libraries
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

// MicroSD Libraries
#include "FS.h"
#include "SD_MMC.h"

unsigned int pictureCount = 0;

// Delay time in millieconds
unsigned int delayTime = 3000;


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
void initMicroSDCard() {
  // Start the MicroSD card

  Serial.println("Mounting MicroSD Card");
  if (!SD_MMC.begin()) {
    Serial.println("MicroSD Card Mount Failed");
    return;
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No MicroSD Card found");
    return;
  }

}

void takeNewPhoto(String path) {
  // Take Picture with Camera

  // Setup frame buffer
  camera_fb_t  * fb = esp_camera_fb_get();

  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // Save picture to microSD card
  fs::FS &fs = SD_MMC;
  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in write mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
  }
  // Close the file
  file.close();

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);
}

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
    config.jpeg_quality = 10;
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

  s->set_framesize(s, FRAMESIZE_VGA);
  Serial.print("Initializing the camera module...");
  Serial.println("Camera OK!");

  // Initialize the MicroSD
  Serial.print("Initializing the MicroSD card module... ");
  initMicroSDCard();

  Serial.print("Delay Time = ");
  Serial.print(delayTime);
  Serial.println(" ms");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  //========================================================================
  /* Assign the api key (required) */
  // configs.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  // configs.database_url = DATABASE_URL;

  /* Sign up */
  // if (Firebase.signUp(&configs, &auth, "", "")) {
  //   Serial.println("ok");
  //   signupOK = true;
  // }
  // else {
  //   Serial.printf("%s\n", configs.signer.signupError.message.c_str());
  // }

  /* Assign the callback function for the long running token generation task */
  // configs.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Firebase.begin(&configs, &auth);
  // Firebase.reconnectWiFi(true);
  //=====================================================================
  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Camerafeed = WiFi.localIP().toString();
  Serial.println("' to connect");
}

void loop() {
  // Path where new image will be saved in MicroSD card
  String path = "/image" + String(pictureCount) + ".jpg";
  Serial.printf("Picture file name: %s\n", path.c_str());

  // Take and Save Photo
  takeNewPhoto(path);

  // Increment picture count
  pictureCount++;

  // Delay for specified period
  delay(delayTime);
  // put your main code here, to run repeatedly:
  // if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
  //   sendDataPrevMillis = millis();
  //   count = map(analogRead(2), 0, 4098, 0, 100);
  //  // count=random(100);
  //   // Write an Int number on the database path test/int
  //   if (Firebase.RTDB.setInt(&fbdo, "vacumRobot/battery", count)) {
  //     Serial.println("PASSED");
  //     Serial.println("PATH: " + fbdo.dataPath());
  //     Serial.println("TYPE: " + fbdo.dataType());
  //   }
  //   else {
  //     Serial.println("FAILED");
  //     Serial.println("REASON: " + fbdo.errorReason());
  //   }
  //   //count++;
  //   if (Firebase.RTDB.setString(&fbdo, "vacumRobot/direction", direction)) {
  //     Serial.println("PASSED");
  //     Serial.println("PATH: " + fbdo.dataPath());
  //     Serial.println("TYPE: " + fbdo.dataType());
  //   }
  //   else {
  //     Serial.println("FAILED");
  //     Serial.println("REASON: " + fbdo.errorReason());
  //   }
  // }
}
