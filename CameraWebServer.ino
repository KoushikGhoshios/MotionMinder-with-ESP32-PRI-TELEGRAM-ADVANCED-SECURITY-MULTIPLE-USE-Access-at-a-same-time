
/*
  By Koushik Ghosh
  for Complete project details, contact @koushik97563@gmail.com 
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

/*
 * Koushik Ghosh
 * 
 * 
 * 
 */

/* ======================================== Including the libraries. */
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
/* ======================================== */

/* ======================================== Variables for network. */
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char *ssid = "TP-Link_A062";
const char *password = "82085728"; //--> Enter your WiFi password.
/* ======================================== */
// WiFi connection variables
#define WIFI_TIMEOUT_MS 20000 // 20 second WiFi connection timeout
#define WIFI_RECOVER_TIME_MS 30000 // Wait 30 seconds after a failed connection attempt

/* ======================================== Variables for telegram bot tokens. */
String BOTtoken = "7305196510:AAECvBgBI7YvkzBW92gO2WupbmwSwdCpxm8";  //--> your Bot Token (Get from Botfather).
/* ======================================== */

/* ======================================== @myidbot ID */
// Use @myidbot to find out the chat ID of an individual or a group.
// Also note that you need to click "start" on a bot before it can message you.
//String CHAT_ID = "943472115";
//String CHAT_IDS[] = {"943472115", "5604911041", "6123427668", "1587305333"};
const char* CHAT_IDS[] = {"943472115", "5604911041", "6123427668", "1587305333"};
const int NUM_CHAT_IDS = sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]);
/* ======================================== */

/* ======================================== Initialize WiFiClientSecure. */
WiFiClientSecure clientTCP;
/* ======================================== */

/* ======================================== Initialize UniversalTelegramBot. */
UniversalTelegramBot bot(BOTtoken, clientTCP);
/* ======================================== */

/* ======================================== Defining the camera's GPIO on the ESP32 Cam. */
// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
/* ======================================== */

/* ======================================== Defines HIGH and LOW with ON and OFF (for LED FLash). */
#define ON HIGH
#define OFF LOW
/* ======================================== */

#define FLASH_LED_PIN   4           //--> LED Flash PIN (GPIO 4)
#define PIR_SENSOR_PIN  13          //--> PIR SENSOR PIN (GPIO 12)

#define EEPROM_SIZE     2           //--> Define the number of bytes you want to access

/* ======================================== Variables for millis (Checks for new messages). */
// Checks for new messages every 1 second (1000 ms).
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
/* ======================================== */

/* ======================================== Variables for millis (to stabilize the PIR Sensor). */
int countdown_interval_to_stabilize_PIR_Sensor = 1000;
unsigned long lastTime_countdown_Ran;
byte countdown_to_stabilize_PIR_Sensor = 30;
/* ======================================== */

bool sendPhoto = false;             //--> Variables for photo sending triggers.

bool PIR_Sensor_is_stable = false;  //--> Variable to state that the PIR sensor stabilization time has been completed.

bool boolPIRState = false;
bool motionDetected = false;   //---> motion detection


void IRAM_ATTR detectsMovement() {
  motionDetected = true;
}

/* ________________________________________________________________________________ String function to parse String */
// Source : https://www.electroniclinic.com/reyax-lora-based-multiple-sensors-monitoring-using-arduino/
// This function is used to get the status of sending photos in json.
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;
  
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for sending feedback messages when photos are successful or failed to be sent to telegram. */
void FB_MSG_is_photo_send_successfully (bool state) {
    String send_feedback_message = "";
    if(state == false) {
    send_feedback_message += "🚀 MotionMinder Bot! 📸\n\n";
    send_feedback_message += "ESP32-CAM failed to send photo.\n";
    send_feedback_message += "Suggestion :\n";
    send_feedback_message += "- Please try again.\n";
    send_feedback_message += "- Reset ESP32-CAM.\n";
    send_feedback_message += "- Change FRAMESIZE (see Drop down frame size in void configInitCamera).\n";
    Serial.print(send_feedback_message);
    send_feedback_message += "\n\n";
    send_feedback_message += "/start : to see all commands.";
    //bot.sendMessage(CHAT_ID, send_feedback_message, "");
    for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
      bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
    }
  }else {
     if(boolPIRState == true) {
       Serial.println("Successfully sent photo.");
       send_feedback_message += "🚀 MotionMinder Bot! 📸\n\n";
       send_feedback_message += "The PIR sensor detects objects and movements.\n";
       send_feedback_message += "Photo sent successfully.\n\n";
       send_feedback_message += "/start : to see all commands.";
       //bot.sendMessage(CHAT_ID, send_feedback_message, "");
       for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
       bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
        }
      }
      if(sendPhoto == true) {
       Serial.println("Successfully sent photo.");
       send_feedback_message += "🚀 MotionMinder Bot! 📸\n\n";
       send_feedback_message += "Photo sent successfully.\n\n";
       send_feedback_message += "/start : to see all commands.";
       //bot.sendMessage(CHAT_ID, send_feedback_message, "");
       for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
       bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
       }
      }
    }
}/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to read PIR sensor value (HIGH/1 OR LOW/0) */
bool PIR_State() {
  return digitalRead(PIR_SENSOR_PIN);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine to turn on or off the LED Flash. */
void LEDFlash_State(bool ledState) {
  digitalWrite(FLASH_LED_PIN, ledState);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for setting and saving settings in EEPROM for "capture photo with LED Flash" mode. */
void enable_capture_Photo_With_Flash(bool state) {
  EEPROM.write(0, state);
  EEPROM.commit();
  delay(50);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to read settings in EEPROM for "capture photos with LED Flash" mode. */
bool capture_Photo_With_Flash_state() {
  return EEPROM.read(0);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for setting and saving settings in EEPROM for "capture photos with PIR Sensor" mode. */
void enable_capture_Photo_with_PIR(bool state) {
  EEPROM.write(1, state);
  EEPROM.commit();
  delay(50);
  if (state) {
    attachInterrupt(digitalPinToInterrupt(PIR_SENSOR_PIN), detectsMovement, RISING);
  } else {
    detachInterrupt(digitalPinToInterrupt(PIR_SENSOR_PIN));
  }
}

/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Function to read settings in EEPROM for "capture photos with PIR Sensor" mode.*/
bool capture_Photo_with_PIR_state() {
  return EEPROM.read(1);
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for camera configuration. */
void configInitCamera(){
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


  /* ---------------------------------------- init with high specs to pre-allocate larger buffers. */
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
    config.jpeg_quality = 10;  
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  
    config.fb_count = 1;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- camera init. */
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    Serial.println();
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Drop down frame size for higher initial frame rate (Set the frame size and quality here) */
  /*
   * If the photo sent by the ESP32-CAM is corrupt or the ESP32-CAM fails to send the photo, to resolve it, follow the steps below :
   * - FRAMESIZE settings :
   *   > Change "s->set_framesize(s, FRAMESIZE_UXGA);" to a lower frame size, such as FRAMESIZE_VGA, FRAMESIZE_CIF and so on.
   * 
   * If you have reduced the frame size, but the photo sent by ESP32-CAM is still corrupt or the ESP32-CAM still fails to send the photo,
   * then change the setting "s->set_quality(s, 30);".
   * - set_quality setting :
   *   > The image quality (set_quality) can be a number between 0 and 63.
   *   > Higher numbers mean lower quality.
   *   > Lower numbers mean higher quality.
   *   > Very low numbers for image quality, specially at higher resolution can make the ESP32-CAM to crash or it may not be able to take the photos properly.
   *   > If THE RECEIVED IMAGE IS CORRUPTED OR FAIL TO SEND PHOTOS, try using a larger value in "s->set_quality(s, 30);", such as 25, 30 and so on until 63.
   * 
   * On my ESP32-CAM, if using "FRAMESIZE_UXGA", the set_quality value is 30.
   * After I tested, the settings above are quite stable both for taking photos indoors, outdoors, in conditions with good lighting quality and in conditions of insufficient light.
   */

  /*
   * UXGA   = 1600 x 1200 pixels
   * SXGA   = 1280 x 1024 pixels
   * XGA    = 1024 x 768  pixels
   * SVGA   = 800 x 600   pixels
   * VGA    = 640 x 480   pixels
   * CIF    = 352 x 288   pixels
   * QVGA   = 320 x 240   pixels
   * HQVGA  = 240 x 160   pixels
   * QQVGA  = 160 x 120   pixels
   */
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_SXGA);  //--> FRAMESIZE_ + UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutines to handle what to do after a new message arrives. */
void handleNewMessages(int numNewMessages) {
  Serial.print("Handle New Messages: ");
  Serial.println(numNewMessages);

  /* ---------------------------------------- "For Loop" to check the contents of the newly received message. */
  for (int i = 0; i < numNewMessages; i++) {
    /* ::::::::::::::::: Check ID (ID obtained from IDBot/@myidbot). */
    /*
     * If the chat_id is different from your chat ID (CHAT_ID), it means that someone (that is not you) has sent a message to your bot.
     * If that’s the case, ignore the message and wait for the next message.
     */
    //String chat_id = String(bot.messages[i].chat_id);
    /*if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "⚠️ Authorization Required: To access this bot, kindly reach out to @Koushik9726", "");
      Serial.println("⚠️ Authorization Required: To access this bot, kindly reach out to @Koushik9726");
      Serial.println("------------");
      continue;
    }*/
    String chat_id = String(bot.messages[i].chat_id);

    // Check if the incoming chat_id is NOT in the CHAT_IDS array
    bool isAuthorized = false;
    for (int j = 0; j < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); j++) {
     if (chat_id == CHAT_IDS[j]) {
       isAuthorized = true;
       break;
      }
    }

    if (!isAuthorized) {
     bot.sendMessage(chat_id, "⚠️ Authorization Required: To access this bot, kindly reach out to @Koushik9726", "");
  
     Serial.println("⚠️ Authorization Required: To access this bot, kindly reach out to @Koushik9726");
  
     Serial.println("------------");
  
     continue;
    }

    /* ::::::::::::::::: */

    /* ::::::::::::::::: Print the received message. */
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    
    //Serial.println(text);
    /* ::::::::::::::::: */

    /* ::::::::::::::::: Check conditions based on commands sent from your telegram BOT. */
    // If it receives the "/start" message, we’ll send the valid commands to control the ESP. This is useful if you happen to forget what are the commands to control your board.
    String send_feedback_message = "";
    
    if (text == "/start") {
      send_feedback_message += "🚀 Welcome to Tech Titans' MotionMinder Bot! 📸\n\n";
      send_feedback_message += "Hello , " + from_name + " ! We're thrilled to have you on board our cutting-edge surveillance project.\n\n";
      send_feedback_message += "🔬 You're now interfacing with our state-of-the-art ESP32-CAM with PIR system.\n";
      send_feedback_message += "🌟 This isn't just any camera - it's your gateway to smart, responsive monitoring!\n";
      send_feedback_message += "🎭 Whether you're securing your space or exploring IoT, you're in for an exciting journey.\n\n";
      send_feedback_message += "Ready to take control? Let's dive into what you can do!\n\n";
      send_feedback_message += "🔧 Available Commands:\n\n";
      send_feedback_message += "/capture_photo : takes a new photo\n\n";
      send_feedback_message += "⚙️ Camera Settings:\n";
      send_feedback_message += "/enable_capture_Photo_With_Flash : Turn on LED flash 💡\n";
      send_feedback_message += "/disable_capture_Photo_With_Flash : Turn off LED flash 🔅\n";
      send_feedback_message += "/enable_capture_Photo_with_PIR : Activate motion detection 👁️\n";
      send_feedback_message += "/disable_capture_Photo_with_PIR : Deactivate motion detection 🚫\n\n";
      send_feedback_message += "📊 Current Status:\n";
      if(capture_Photo_With_Flash_state() == ON) {
        send_feedback_message += "- Capture Photo With Flash = ON\n";
      }
      if(capture_Photo_With_Flash_state() == OFF) {
        send_feedback_message += "- Capture Photo With Flash = OFF\n";
      }
      if(capture_Photo_with_PIR_state() == ON) {
        send_feedback_message += "- Capture Photo With PIR = ON\n";
      }
      if(capture_Photo_with_PIR_state() == OFF) {
        send_feedback_message += "- Capture Photo With PIR = OFF\n";
      }
      if(PIR_Sensor_is_stable == false) {
        send_feedback_message += "\n🔄 PIR Sensor Calibrating:\n";
        send_feedback_message += "The PIR sensor is being stabilized.\n";
        send_feedback_message += "Stabilization time is " + String(countdown_to_stabilize_PIR_Sensor) + " seconds away. Please wait.\n\n";
      }
      send_feedback_message += "Let's get started! What would you like to do first?\n";
      //bot.sendMessage(CHAT_ID, send_feedback_message, "");
      for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
       bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
      }
      Serial.println("------------");
    }
    
    // The condition if the command received is "/capture_photo".
    else if (text == "/capture_photo") {
      sendPhoto = true;
      Serial.println("New photo request");
    }
    
    // The condition if the command received is "/enable_capture_Photo_With_Flash".
    else if (text == "/enable_capture_Photo_With_Flash") {
      enable_capture_Photo_With_Flash(ON);
      String send_feedback_message = "🚀 MotionMinder Bot! 📸\n\nCapture Photo With Flash = ON\n\n";
      // Take a test photo with flash to confirm it's working
      sendPhoto = true;
      send_feedback_message += "Taking a test photo with flash...\n\n";
      send_feedback_message += "/start : to see all commands.";
      //String send_feedback_message = "From the ESP32-CAM :\n\nCapture Photo With Flash = ON\n\n/start : to see all commands.";
      //bot.sendMessage(CHAT_ID, send_feedback_message, "");
      for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
      bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
      }
    }
    
    // The condition if the command received is "/disable_capture_Photo_With_Flash".
    else if (text == "/disable_capture_Photo_With_Flash") {
      enable_capture_Photo_With_Flash(OFF);
      String send_feedback_message = "🚀 MotionMinder Bot! 📸\n\nCapture Photo With Flash = OFF\n\n";
      // Take a test photo without flash to confirm it's working
      sendPhoto = true;
      send_feedback_message += "Taking a test photo without flash...\n\n";
      send_feedback_message += "/start : to see all commands.";
      //String send_feedback_message = "From the ESP32-CAM :\n\nCapture Photo With Flash = OFF\n\n/start : to see all commands.";
      //bot.sendMessage(CHAT_ID, send_feedback_message, "");
      for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
      bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
      }
    }

    // The condition if the command received is "/enable_capture_Photo_with_PIR".
    else if (text == "/enable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(ON);
      botRequestDelay = 10000; // Increase delay when PIR is active
      String send_feedback_message = "🚀 MotionMinder Bot! 📸\n\nCapture Photo With PIR = ON\n\n";
      if (PIR_Sensor_is_stable) {
        send_feedback_message += "PIR sensor is ready and monitoring.\n\n";
      } else {
        send_feedback_message += "PIR sensor is still stabilizing. Please wait.\n\n";
      }
      send_feedback_message += "/start : to see all commands.";
      //String send_feedback_message = "From the ESP32-CAM :\n\nCapture Photo With PIR = ON\n\n/start : to see all commands.";
      //bot.sendMessage(CHAT_ID, send_feedback_message, "");
      for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
      bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
      }
    }

    // The condition if the command received is "/disable_capture_Photo_with_PIR".
    else if (text == "/disable_capture_Photo_with_PIR") {
      enable_capture_Photo_with_PIR(OFF);
      botRequestDelay = 1000; // Reset delay when PIR is disabled
      String send_feedback_message = "🚀 MotionMinder Bot! 📸\n\nCapture Photo With PIR = OFF\n\n";
      send_feedback_message += "/start : to see all commands.";
      //String send_feedback_message = "From the ESP32-CAM :\n\nCapture Photo With PIR = OFF\n\n/start : to see all commands.";
      //bot.sendMessage(CHAT_ID, send_feedback_message, "");
      for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
      bot.sendMessage(CHAT_IDS[i], send_feedback_message, "");
      }
    }
    /* ::::::::::::::::: */
  }
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ Subroutine for the process of taking and sending photos. */
String sendPhotoTelegram() {
  const char* myDomain = "api.telegram.org";
  String getAll = "";
  String getBody = "";

  /* ---------------------------------------- The process of taking photos. */
  Serial.println("Taking a photo...");

  /* ::::::::::::::::: Turns on LED FLash if setting is "enable_capture_Photo_With_Flash(ON);". */
  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(ON);
  }
  delay(1500);
  /* ::::::::::::::::: */
  
  /* ::::::::::::::::: Taking a photo. */ 
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    Serial.println("Restart ESP32 Cam");
    delay(1000);
    ESP.restart();
    return "Camera capture failed";
  }  
  /* ::::::::::::::::: */

  /* ::::::::::::::::: Turn off the LED Flash after successfully taking photos. */
  if(capture_Photo_With_Flash_state() == ON) {
    LEDFlash_State(OFF);
  }
  /* ::::::::::::::::: */
  Serial.println("Successful photo taking.");
  /* ---------------------------------------- */
  

  /* ---------------------------------------- The process of sending photos. */
  Serial.println("Connect to " + String(myDomain));

  if (clientTCP.connect(myDomain, 443)) {
    Serial.println("Connection successful");
    Serial.print("Send photos");
    
    String head = "--Esp32Cam\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n";
    for (int i = 0; i < NUM_CHAT_IDS; i++) {
     head += CHAT_IDS[i];
      if (i < NUM_CHAT_IDS - 1) {
        head += ",";
      }
    }

    head += "\r\n--Esp32Cam\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Esp32Cam--\r\n";

    /* ::::::::::::::::: If you only use low framesize, such as CIF, QVGA, HQVGA and QQVGA, then use the variables below to save more memory. */
    //uint16_t imageLen = fb->len;
    //uint16_t extraLen = head.length() + tail.length();
    //uint16_t totalLen = imageLen + extraLen;
    /* ::::::::::::::::: */

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;
  
    clientTCP.println("POST /bot"+BOTtoken+"/sendPhoto HTTP/1.1");
    clientTCP.println("Host: " + String(myDomain));
    clientTCP.println("Content-Length: " + String(totalLen));
    clientTCP.println("Content-Type: multipart/form-data; boundary=Esp32Cam");
    clientTCP.println();
    clientTCP.print(head);
  
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        clientTCP.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        clientTCP.write(fbBuf, remainder);
      }
    }  
    
    clientTCP.print(tail);
    
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   //--> timeout 10 seconds (To send photos.)
    long startTimer = millis();
    boolean state = false;
    
    while ((startTimer + waitTime) > millis()){
      Serial.print(".");
      delay(100);      
      while (clientTCP.available()) {
        char c = clientTCP.read();
        if (state==true) getBody += String(c);        
        if (c == '\n') {
          if (getAll.length()==0) state=true; 
          getAll = "";
        } 
        else if (c != '\r')
          getAll += String(c);
        startTimer = millis();
      }
      if (getBody.length()>0) break;
    }
    clientTCP.stop();
    Serial.println(getBody);

    /* ::::::::::::::::: The condition to check if the photo was sent successfully or failed. */
    // If the photo is successful or failed to send, a feedback message will be sent to Telegram.
    if(getBody.length() > 0) {
      String send_status = "";
      send_status = getValue(getBody, ',', 0);
      send_status = send_status.substring(6);
      
      if(send_status == "true") {
        FB_MSG_is_photo_send_successfully(true);  //--> The photo was successfully sent and sent an information message that the photo was successfully sent to telegram.
      }
      if(send_status == "false") {
        FB_MSG_is_photo_send_successfully(false); //--> The photo failed to send and sends an information message that the photo failed to send to telegram.
      }
    }
    if(getBody.length() == 0) FB_MSG_is_photo_send_successfully(false); //--> The photo failed to send and sends an information message that the photo failed to send to telegram.
    /* ::::::::::::::::: */
  }
  else {
    getBody="Connected to api.telegram.org failed.";
    Serial.println("Connected to api.telegram.org failed.");
  }
  return getBody;
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID SETTUP() */
void setupWiFi()                                             // Add the new functions here, just before setup()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && 
         millis() - startAttemptTime < WIFI_TIMEOUT_MS){}

  if(WiFi.status() != WL_CONNECTED){
    Serial.println("WIFI Connection FAILED");
    delay(WIFI_RECOVER_TIME_MS);
    ESP.restart();
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void checkWiFiConnection()
{
  if(WiFi.status() != WL_CONNECTED){
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startAttemptTime < WIFI_TIMEOUT_MS){}
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("WiFi reconnection failed. Restarting...");
      delay(WIFI_RECOVER_TIME_MS);
      ESP.restart();
    } else {
      Serial.println("WiFi reconnected successfully");
    }
  }
}




void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //--> Disable brownout detector.

   /* ---------------------------------------- Init serial communication speed (baud rate). */
  Serial.begin(115200);
  delay(1000);
  /* ---------------------------------------- */

  Serial.println();
  Serial.println();
  Serial.println("------------");

  /* ---------------------------------------- Starts the EEPROM, writes and reads the settings stored in the EEPROM. */
  EEPROM.begin(EEPROM_SIZE);

  /* ::::::::::::::::: Writes settings to EEPROM. */
  /*
   * Activate the lines of code below for 1 time only.
   * After you upload the code, then "comment" the lines of code below, then upload the code again.
   */
  //enable_capture_Photo_With_Flash(OFF);
  //enable_capture_Photo_with_PIR(OFF);
  //delay(500);
  /* ::::::::::::::::: */

  Serial.println("📊 Current Status:");
  if(capture_Photo_With_Flash_state() == ON) {
    Serial.println("- Capture Photo With Flash = ON");
  }
  if(capture_Photo_With_Flash_state() == OFF) {
    Serial.println("- Capture Photo With Flash = OFF");
  }
  if(capture_Photo_with_PIR_state() == ON) {
    Serial.println("- Capture Photo With PIR = ON");
    botRequestDelay = 10000;
  }
  if(capture_Photo_with_PIR_state() == OFF) {
    Serial.println("- Capture Photo With PIR = OFF");
    botRequestDelay = 1000;
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Set LED Flash as output and make the initial state of the LED Flash is off. */
  pinMode(FLASH_LED_PIN, OUTPUT);
  LEDFlash_State(OFF);
  /* ---------------------------------------- */
  pinMode(PIR_SENSOR_PIN, INPUT_PULLUP);
  enable_capture_Photo_with_PIR(capture_Photo_with_PIR_state());

  /* ---------------------------------------- Config and init the camera. */
  Serial.println();
  Serial.println("Start configuring and initializing the camera...");
  configInitCamera();
  Serial.println("Successfully configure and initialize the camera.");
  Serial.println();
  /* ---------------------------------------- */

  /* ---------------------------------------- Connect to Wi-Fi. */
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  setupWiFi();
  clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); //--> Add root certificate for api.telegram.org

  /* ::::::::::::::::: The process of connecting ESP32 CAM with WiFi Hotspot / WiFi Router. */
  /*
   * The process timeout of connecting ESP32 CAM with WiFi Hotspot / WiFi Router is 20 seconds.
   * If within 20 seconds the ESP32 CAM has not been successfully connected to WiFi, the ESP32 CAM will restart.
   * I made this condition because on my ESP32-CAM, there are times when it seems like it can't connect to WiFi, so it needs to be restarted to be able to connect to WiFi.
   */
  int connecting_process_timed_out = 20; //--> 20 = 20 seconds.
  connecting_process_timed_out = connecting_process_timed_out * 2;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    LEDFlash_State(ON);
    delay(250);
    LEDFlash_State(OFF);
    delay(250);
    if(connecting_process_timed_out > 0) connecting_process_timed_out--;
    if(connecting_process_timed_out == 0) {
      delay(1000);
      ESP.restart();
    }
  }
  /* ::::::::::::::::: */
  
  LEDFlash_State(OFF);
  Serial.println();
  Serial.print("Successfully connected to ");
  Serial.println(ssid);
  Serial.print("ESP32-CAM IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  Serial.println("The PIR sensor is being stabilized.");
  Serial.printf("Stabilization time is %d seconds away. Please wait.\n", countdown_to_stabilize_PIR_Sensor);
  
  Serial.println("------------");
  Serial.println();
  /* ---------------------------------------- */
}
/* ________________________________________________________________________________ */

/* ________________________________________________________________________________ VOID LOOP() */
void loop() {
  /* ---------------------------------------- Conditions for taking and sending photos. */
  checkWiFiConnection();  // Check WiFi connection at the start of each loop
  if(sendPhoto) {
    Serial.println("Preparing photo...");
    sendPhotoTelegram(); 
    sendPhoto = false; 
  }

  if (motionDetected) {
    sendPhotoTelegram();
    Serial.println("Motion Detected");
    motionDetected = false;
  }



  if(capture_Photo_with_PIR_state() == ON) {
    if(PIR_State() == true && PIR_Sensor_is_stable == true) {
      Serial.println("------------");
      Serial.println("The PIR sensor detects objects and movements.");
      boolPIRState = true;
      sendPhotoTelegram();
      boolPIRState = false;
    }
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Condition to check if any new messages come in. */
  /*
   * Checks for a new message every 1 second (see the "botRequestDelay" variable) if photo capture with PIR is not enabled / OFF.
   * 
   * If photo capture with PIR is on, it will check for new messages every 20 seconds. I made this condition because to 
   * check for new messages takes about 2-4 seconds (depending on internet network speed) and it can delay the PIR sensor reading process.
   * So by checking for new messages every 20 seconds, the PIR sensor readings can be prioritized.
   */
  if(millis() > lastTimeBotRan + botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
  /* ---------------------------------------- */
  
  /* ---------------------------------------- Conditions to stabilize the PIR Sensor. */
  /*
   * This condition is created because when the PIR sensor is turned on, it needs a stabilization time of about 30-60 seconds,
   * so that it can detect correctly or detect with little noise.
   * 
   * With this condition, the sensor can only work after the stabilization time is complete.
   * 
   * The stabilization time used in this project is 30 seconds (The 30 second time may not be accurate because it was interrupted by another process).
   * You can change this by changing the value of the "countdown_to_stabilize_PIR_Sensor" variable.
   */
  if(PIR_Sensor_is_stable == false) {
    if(millis() > lastTime_countdown_Ran + countdown_interval_to_stabilize_PIR_Sensor) {
      if(countdown_to_stabilize_PIR_Sensor > 0) countdown_to_stabilize_PIR_Sensor--;
      if(countdown_to_stabilize_PIR_Sensor == 0) {
        PIR_Sensor_is_stable = true;
        Serial.println();
        Serial.println("------------");
        Serial.println("The PIR sensor stabilization period is complete");
        Serial.println("The sensor is now ready to function");
        Serial.println("------------");
        String send_Status_PIR_Sensor = "";
        send_Status_PIR_Sensor += "🚀 MotionMinder Bot! 📸\n\n";
        send_Status_PIR_Sensor += "The PIR sensor stabilization period is complete\n";
        send_Status_PIR_Sensor += "The sensor is now ready to function";
        //bot.sendMessage(CHAT_ID, send_Status_PIR_Sensor, "");
        //String send_feedback_message = "🚀 MotionMinder Bot! 📸\n\nThe PIR sensor stabilization period is complete\nThe sensor is now ready to function";
        for (int i = 0; i < sizeof(CHAT_IDS) / sizeof(CHAT_IDS[0]); i++) {
         bot.sendMessage(CHAT_IDS[i], send_Status_PIR_Sensor, "");
        }
      }
      lastTime_countdown_Ran = millis();
    }
  }
  /* ---------------------------------------- */

  /* ---------------------------------------- Conditions for reading the PIR sensor and executing its commands. */
  /*
   * "capture_Photo_with_PIR_state()" must be "ON" to take photos with the PIR sensor.
   * You can set it from Telegram messages/commands.
   */
  
}
