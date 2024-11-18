#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <PhoneDTMF.h>
#include <driver/adc.h>
#include <ArduinoOTA.h>

#define RING_PIN 18
#define AUDIO_IN_PIN 34
PhoneDTMF dtmf = PhoneDTMF(128);

// State Vars
bool ringState = false;
char lastDigitPressed = 'z';
unsigned long digitResetMillis = LONG_MAX;

// Wifi Settings
const char* ssid = "Old Greg 2.4";
const char* password = "yoloswag";

AsyncWebServer server(80);

// Ring function
void ring() {
    while (ringState == true) {
      int period = 40000;
      for (int i = 0; i < 50; i++)
      {
        digitalWrite(RING_PIN, HIGH); // Set the pin HIGH
        delayMicroseconds(period / 2); // Half-period delay (20ms)
        digitalWrite(RING_PIN, LOW); // Set the pin LOW
        delayMicroseconds(period / 2); // Half-period delay (20ms)
      
      }
      delay(1500);
    }
}

void setup(){
  Serial.begin(115200);

  // setup RING pin
  pinMode(RING_PIN, OUTPUT);
  digitalWrite(RING_PIN, LOW);

  // setup DTMF listener
  pinMode(AUDIO_IN_PIN, INPUT);
  adc1_config_width(ADC_WIDTH_BIT_12); // set 12 bit (0-4096)
  adc1_config_channel_atten(ADC1_CHANNEL_5, ADC_ATTEN_DB_0); // do not use attenuation
  dtmf.begin(AUDIO_IN_PIN);

  WiFi.mode(WIFI_STA); // Set in Station Mode
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
 }

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      }
    });

  ArduinoOTA.begin();

  // Serve the web page with buttons
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html lang='en'><head><meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>ESP32 LED Control</title></head><body><h1>ESP32 SLIC Ring Control</h1>";
    html += "<p>Ring State: " + String(ringState ? "ON" : "OFF") + "</p>";
    html += "<p><a href=\"/ring/on\"><button>Turn On</button></a></p>";
    html += "<p><a href=\"/ring/off\"><button>Turn Off</button></a></p>";
    html += "</body></html>";
    request->send(200, "text/html", html);
  });


    // Turn Ring on
  server.on("/ring/on", HTTP_GET, [](AsyncWebServerRequest *request) {
    ringState = true;
    request->redirect("/");
  });

  // Turn LED off
  server.on("/ring/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    ringState = false;
    request->redirect("/");
  });

  // Start the server
  server.begin();

  Serial.println("\nConnected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

void loop()
{
  ArduinoOTA.handle();

  char button = dtmf.tone2char(dtmf.detect());

  if (millis() > digitResetMillis) {
    digitResetMillis = LONG_MAX;
    lastDigitPressed = 'z';
  }

  if(button > 0) {
    if (button != lastDigitPressed) {
      lastDigitPressed = button;
      Serial.print(lastDigitPressed); Serial.println(" pressed");
    }
    digitResetMillis = millis() + 80;
  }
  ring();
}