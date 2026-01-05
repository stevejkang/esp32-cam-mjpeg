#include "esp_camera.h"
#include <WiFi.h>
#include "esp_timer.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "secrets.h"

// AI-Thinker ESP32-CAM Pin Definition
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

WiFiServer server(80);

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

bool initCamera() {
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

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // 1600x1200
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA; // 800x600
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera initialization failed: 0x%x\n", err);
    return false;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  sensor->set_framesize(sensor, FRAMESIZE_SVGA); // 800x600
  sensor->set_vflip(sensor, 1); // vertical flip
  sensor->set_hmirror(sensor, 1); // horizontal mirror
  
  Serial.println("Camera initialization completed");
  return true;
}

bool initWiFi() {
  Serial.print("Connecting to WiFi...");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setSleep(false); // Disable WiFi sleep mode
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed");
    return false;
  }
  
  Serial.println("WiFi connection successful!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Stream URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/stream");

  return true;
}

void handleStream(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.printf("Content-Type: %s\r\n", STREAM_CONTENT_TYPE);
  client.println("Access-Control-Allow-Origin: *");
  client.println();
  
  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    
    if (!fb) {
      Serial.println("Frame capture failed");
      continue;
    }

    client.print(STREAM_BOUNDARY);
    client.printf(STREAM_PART, fb->len);
    client.write(fb->buf, fb->len);
    
    esp_camera_fb_return(fb);
    
    if (!client.connected()) {
      break;
    }
  }
  
  Serial.println("Stream ended");
}

void handleHealthCheck(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println();
  client.println("OK");
}

void handleNotFound(WiFiClient& client) {
  client.println("HTTP/1.1 404 Not Found");
  client.println("Content-Type: text/plain");
  client.println();
  client.println("Not Found");
}

void setup() {
  // Brownout protection
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  if (!initCamera()) {
    Serial.println("System stopped: Camera initialization failed");
    while (true) {
      delay(1000);
    }
  }
  
  if (!initWiFi()) {
    Serial.println("System stopped: WiFi connection failed");
    while (true) {
      delay(1000);
    }
  }
  
  server.begin();
  Serial.println("Successfully started");
}

void loop() {
  WiFiClient client = server.available();
  
  if (client) {
    String request = "";

    while (client.connected() && client.available()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("GET ")) {
        request = line;
      }
      if (line == "\r") {
        break;
      }
    }

    String path = "";
    if (request.startsWith("GET ")) {
      int start = 4;
      int end = request.indexOf(" HTTP/");
      if (end > start) {
        path = request.substring(start, end);
      }
    }

    if (path == "/stream") {
      handleStream(client);
    } else if (path == "/" || path == "") {
      handleHealthCheck(client);
    } else {
      handleNotFound(client);
    }
    
    client.stop();
    Serial.println("Client disconnected");
  }
  
  delay(1);
}
