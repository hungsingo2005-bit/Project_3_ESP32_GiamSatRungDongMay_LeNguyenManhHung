#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <MPU6050.h>
#include <HTTPClient.h>   // THÊM

// ===== WIFI =====
const char* ssid = "Hào Hùng Phát 2";
const char* password = "Lenguyenmanhhung0901";

// const char* ssid = "Hùnn";
// const char* password = "11223301";

// ===== MQTT =====
const char* mqtt_server = "c542be78d0cb4caba47847e0f90e97f5.s1.eu.hivemq.cloud";
const char* mqtt_user = "lenguyenmanhhung";
const char* mqtt_pass = "Manhhung05";

// ===== TELEGRAM =====
String BOT_TOKEN = "8163882798:AAH99a5iqdJ666oAPlRc-OJ1tBlQP8B60Jk";   // TELEGRAM
String CHAT_ID = "5920860822";                                        // TELEGRAM

// ===== CHÂN =====
#define LED_PIN 2
#define LED_GREEN 5
#define LED_YELLOW 18
#define LED_RED 19
#define BUZZER 23

WiFiClientSecure espClient;
PubSubClient client(espClient);
MPU6050 mpu;

// ===== BIẾN =====
bool sendData = true;
int16_t ax, ay, az;
int last_ax = 0, last_ay = 0;

bool buzzerState = false;
bool redState = false;
unsigned long lastBeepTime = 0;
unsigned long lastSendTime = 0;   // TELEGRAM chống spam

// ===== TELEGRAM FUNCTION =====
// void sendTelegram(String message) {
//   WiFiClientSecure client;
//   client.setInsecure();

//   HTTPClient https;

//   String url = "https://api.telegram.org/bot" + BOT_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + message;

//   Serial.println("Sending Telegram...");

//   if (https.begin(client, url)) {
//     int httpCode = https.GET();

//     if (httpCode > 0) {
//       Serial.println("Telegram sent ✅");
//     } else {
//       Serial.println("Send failed ❌");
//     }

//     https.end();
//   } else {
//     Serial.println("HTTPS begin failed ❌");
//   }
// }
void sendTelegram(String message) {
  WiFiClientSecure client;
  client.setInsecure();

  // 🔥 encode đơn giản
  message.replace(" ", "%20");

  String url = "https://api.telegram.org/bot" + BOT_TOKEN +
               "/sendMessage?chat_id=" + CHAT_ID +
               "&text=" + message;

  // Serial.println("Sending Telegram...");
  // Serial.println(url);   // DEBUG

  HTTPClient https;

  if (https.begin(client, url)) {
    int httpCode = https.GET();

    // Serial.print("HTTP Code: ");
    // Serial.println(httpCode);

    String payload = https.getString();
    // Serial.println("Response:");
    // Serial.println(payload);

    https.end();
  } else {
    Serial.println("HTTPS begin FAILED ❌");
  }
}

// ===== WIFI =====
void setup_wifi() {
  Serial.print("Connecting WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

// ===== MQTT CALLBACK =====
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (int i = 0; i < length; i++) msg += (char)payload[i];

  msg.trim();
  Serial.println(msg);

  if (msg == "OFF" || msg == "STOP") {
    sendData = false;
  } else if (msg == "ON" || msg == "START") {
    sendData = true;
  }
}

// ===== MQTT CONNECT =====
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");
    String clientId = "ESP32_HUNG" + String(random(1000, 9999));

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected!");
      client.subscribe("hung123/control");
    } else {
      Serial.println("retry...");
      delay(2000);
    }
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  setup_wifi();

  espClient.setInsecure();
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);

  Wire.begin(21, 22);
  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 OK");
  } else {
    Serial.println("MPU6050 FAIL");
  }
}

// ===== LOOP =====
void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  if (sendData) {

    mpu.getAcceleration(&ax, &ay, &az);

    int diff_ax = abs(ax - last_ax);
    int diff_ay = abs(ay - last_ay);

    if (diff_ax < 400) diff_ax = 0;
    if (diff_ay < 400) diff_ay = 0;

    bool strongVibration = false;
    String status;

    if (diff_ax > 3700 || diff_ay > 3700) {
      status = "RUNG MANH";
      strongVibration = true;
      digitalWrite(LED_PIN, HIGH);
    } 
    else if (diff_ax > 1200 || diff_ay > 1200) {
      status = "RUNG NHE";
      digitalWrite(LED_PIN, HIGH);
    } 
    else {
      status = "BINH THUONG";
      digitalWrite(LED_PIN, HIGH);
    }

    // 🟢 Bình thường
    if (status == "BINH THUONG") {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_YELLOW, LOW);
      digitalWrite(LED_RED, LOW);
      digitalWrite(BUZZER, LOW);
      buzzerState = false;
    }

    // 🟡 Rung nhẹ
    else if (status == "RUNG NHE") {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, HIGH);
      digitalWrite(LED_RED, LOW);
      digitalWrite(BUZZER, LOW);
    }

    // 🔴 Rung mạnh
    else if (strongVibration) {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_YELLOW, LOW);

      // ===== TELEGRAM SEND =====
      if (millis() - lastSendTime > 5000) {
        sendTelegram("CANH BAO: MAY RUNG MANH!");
        lastSendTime = millis();
      }

      // Nhấp nháy LED + còi
      if (millis() - lastBeepTime > 200) {
        redState = !redState;
        buzzerState = !buzzerState;

        digitalWrite(LED_RED, redState);
        digitalWrite(BUZZER, buzzerState);

        lastBeepTime = millis();
      }
    }

    last_ax = ax;
    last_ay = ay;

    char msg[120];
    sprintf(msg,
      "\"ax\":%d,\"ay\":%d,\"az\":%d,\"%s\"",
      ax, ay, az, status.c_str()
    );

    client.publish("hung123/test", msg);
    client.publish("hung123/ax", String(ax).c_str());
    client.publish("hung123/ay", String(ay).c_str());
    client.publish("hung123/az", String(az).c_str());

    Serial.println(msg);
  }

  delay(400);
}