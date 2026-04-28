#include "esp_camera.h"
#include "board_config.h"
#include <shapes_inferencing.h>
#include "edge-impulse-sdk/dsp/image/image.hpp"
#include <WiFi.h>
#include <WebServer.h>

/* WiFi credentials */
const char* WIFI_SSID = "Sheth";
const char* WIFI_PASS = "123456789";

/* Private variables */
static bool debug_nn = false;
uint8_t *snapshot_buf;

#define EI_CAMERA_RAW_FRAME_BUFFER_COLS  320
#define EI_CAMERA_RAW_FRAME_BUFFER_ROWS  240
#define EI_CAMERA_FRAME_BYTE_SIZE        3

#define MAX_RESULTS  50
#define VOTE_SAMPLES  5

WebServer server(80);

// ── Last captured JPEG ─────────────────────────────────
uint8_t *lastJpegBuf = nullptr;
size_t   lastJpegLen = 0;

// ── Result ring buffer ─────────────────────────────────
struct Result {
  String label;
  String timestamp;
};
Result results[MAX_RESULTS];
int resultCount = 0;
int resultHead  = 0;

void addResult(const String& label) {
  int idx = (resultHead + resultCount) % MAX_RESULTS;
  results[idx].label     = label;
  results[idx].timestamp = String(millis() / 1000) + "s";

  if (resultCount < MAX_RESULTS) {
    resultCount++;
  } else {
    resultHead = (resultHead + 1) % MAX_RESULTS;
  }
}

// ── HTTP handlers ──────────────────────────────────────
void handleImage() {
  if (lastJpegBuf == nullptr || lastJpegLen == 0) {
    server.send(404, "text/plain", "No image captured yet");
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store");
  server.send_P(200, "image/jpeg", (const char*)lastJpegBuf, lastJpegLen);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta http-equiv='refresh' content='3'>"
    "<title>ESP32 Classifier</title>"
    "<style>"
    "body{font-family:monospace;background:#111;color:#0f0;padding:20px;}"
    "h1{color:#fff;}"
    "img{display:block;margin:16px 0;border:2px solid #333;width:320px;height:240px;}"
    "table{border-collapse:collapse;width:100%;max-width:500px;margin-top:16px;}"
    "th,td{border:1px solid #333;padding:8px 16px;text-align:left;}"
    "th{background:#222;color:#aaa;}"
    "tr:last-child td{color:#ff0;}"
    "</style>"
    "</head>"
    "<body>"
    "<h1>Classification Results</h1>"
    "<p style='color:#555'>Auto-refreshes every 3s &nbsp;|&nbsp; ";

  html += String(resultCount) + " result(s) stored</p>";
  html += "<img src='/image' alt='Last captured frame'>";
  html += "<table><tr><th>#</th><th>Uptime</th><th>Label</th></tr>";

  for (int i = resultCount - 1; i >= 0; i--) {
    int idx = (resultHead + i) % MAX_RESULTS;
    html += "<tr><td>" + String(i + 1) + "</td>"
          + "<td>" + results[idx].timestamp + "</td>"
          + "<td>" + results[idx].label     + "</td></tr>";
  }

  html += "</table></body></html>";
  server.send(200, "text/html", html);
}

void handleJson() {
  String json = "[";
  for (int i = resultCount - 1; i >= 0; i--) {
    int idx = (resultHead + i) % MAX_RESULTS;
    if (i < resultCount - 1) json += ",";
    json += "{\"label\":\"" + results[idx].label
          + "\",\"t\":\""  + results[idx].timestamp + "\"}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

// ── WiFi + web server init ─────────────────────────────
void initWiFi() {
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nWiFi connected. IP: %s\n", WiFi.localIP().toString().c_str());

  server.on("/",      handleRoot);
  server.on("/json",  handleJson);
  server.on("/image", handleImage);
  server.begin();
  Serial.println("Web server started.");
}

// ── Camera init ────────────────────────────────────────
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count     = 1;
  config.fb_location  = CAMERA_FB_IN_PSRAM;
  config.grab_mode    = CAMERA_GRAB_WHEN_EMPTY;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    while (true) delay(1000);
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
  s->set_awb_gain(s, 1);

  Serial.println("Camera initialised");
}

// ── Edge Impulse data callback ─────────────────────────
static int ei_camera_get_data(size_t offset, size_t length, float *out_ptr) {
  size_t pixel_ix    = offset * 3;
  size_t pixels_left = length;
  size_t out_ptr_ix  = 0;

  while (pixels_left != 0) {
    out_ptr[out_ptr_ix] = (snapshot_buf[pixel_ix + 2] << 16)
                        + (snapshot_buf[pixel_ix + 1] << 8)
                        +  snapshot_buf[pixel_ix];
    out_ptr_ix++;
    pixel_ix += 3;
    pixels_left--;
  }
  return 0;
}

// ── Run classifier on one frame, return best label ─────
String classifyOnce(bool saveJpeg) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("# ERR: Frame capture failed");
    return "error";
  }

  if (saveJpeg) {
    if (lastJpegBuf) { free(lastJpegBuf); lastJpegBuf = nullptr; lastJpegLen = 0; }
    lastJpegBuf = (uint8_t*)malloc(fb->len);
    if (lastJpegBuf) {
      memcpy(lastJpegBuf, fb->buf, fb->len);
      lastJpegLen = fb->len;
    } else {
      Serial.println("# WARN: Could not allocate JPEG buffer");
    }
  }

  snapshot_buf = (uint8_t*)malloc(
    EI_CAMERA_RAW_FRAME_BUFFER_COLS *
    EI_CAMERA_RAW_FRAME_BUFFER_ROWS *
    EI_CAMERA_FRAME_BYTE_SIZE
  );
  if (!snapshot_buf) {
    Serial.println("# ERR: Failed to allocate snapshot buffer");
    esp_camera_fb_return(fb);
    return "error";
  }

  bool converted = fmt2rgb888(fb->buf, fb->len, PIXFORMAT_JPEG, snapshot_buf);
  esp_camera_fb_return(fb);

  if (!converted) {
    Serial.println("# ERR: JPEG to RGB888 conversion failed");
    free(snapshot_buf);
    return "error";
  }

  if (EI_CLASSIFIER_INPUT_WIDTH  != EI_CAMERA_RAW_FRAME_BUFFER_COLS ||
      EI_CLASSIFIER_INPUT_HEIGHT != EI_CAMERA_RAW_FRAME_BUFFER_ROWS) {
    ei::image::processing::crop_and_interpolate_rgb888(
      snapshot_buf,
      EI_CAMERA_RAW_FRAME_BUFFER_COLS,
      EI_CAMERA_RAW_FRAME_BUFFER_ROWS,
      snapshot_buf,
      EI_CLASSIFIER_INPUT_WIDTH,
      EI_CLASSIFIER_INPUT_HEIGHT
    );
  }

  ei::signal_t signal;
  signal.total_length = EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT;
  signal.get_data     = &ei_camera_get_data;

  ei_impulse_result_t result = { 0 };
  EI_IMPULSE_ERROR err = run_classifier(&signal, &result, debug_nn);

  free(snapshot_buf);

  if (err != EI_IMPULSE_OK) {
    Serial.printf("# ERR: Classifier failed (%d)\n", err);
    return "error";
  }

  String bestLabel = "unknown";
  float  bestScore = -1.0f;

#if EI_CLASSIFIER_OBJECT_DETECTION == 1
  for (uint32_t i = 0; i < result.bounding_boxes_count; i++) {
    ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
    if (bb.value > bestScore) { bestScore = bb.value; bestLabel = String(bb.label); }
  }
#else
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > bestScore) {
      bestScore = result.classification[i].value;
      bestLabel = String(ei_classifier_inferencing_categories[i]);
    }
  }
#endif

  return bestLabel;
}

// ── Majority-vote across VOTE_SAMPLES shots ────────────
String captureAndClassify() {
  String votes[VOTE_SAMPLES];

  for (int i = 0; i < VOTE_SAMPLES; i++) {
    bool isLast = (i == VOTE_SAMPLES - 1);
    votes[i] = classifyOnce(isLast);
    Serial.printf("# Vote %d/%d: %s\n", i + 1, VOTE_SAMPLES, votes[i].c_str());
    server.handleClient();
  }

  String winnerLabel = "unknown";
  int    winnerCount = 0;

  for (int i = 0; i < VOTE_SAMPLES; i++) {
    if (votes[i] == "error") continue;
    int count = 0;
    for (int j = 0; j < VOTE_SAMPLES; j++) {
      if (votes[j] == votes[i]) count++;
    }
    if (count > winnerCount) {
      winnerCount = count;
      winnerLabel = votes[i];
    }
  }

  Serial.printf("# Majority: %s (%d/%d)\n", winnerLabel.c_str(), winnerCount, VOTE_SAMPLES);
  return winnerLabel;
}

// ── Arduino entry points ───────────────────────────────
void setup() {
  Serial.begin(115200);
  while (!Serial);
  initWiFi();
  initCamera();
  Serial.println("# Ready. Send 'active' to trigger inference.");
}

void loop() {
  server.handleClient();

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("active")) {
      String label = captureAndClassify();
      Serial.println(label);  // ← only this line goes to the sensor board
      addResult(label);
    }
  }
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_CAMERA
#error "Invalid model for current sensor"
#endif
