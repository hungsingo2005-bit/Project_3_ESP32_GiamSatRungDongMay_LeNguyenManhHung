#include <WiFi.h>
#include <PubSubClient.h>

// ====== WIFI ======
const char* ssid = "Hào Hùng Phát 2";
const char* password = "Lenguyenmanhhung0901";

// ====== MQTT ======
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

// ====== NEW: biến điều khiển gửi dữ liệu ======
bool sendData = true;   // NEW

// ====== CONNECT WIFI ======
void setup_wifi() {
  Serial.print("Connecting WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
}

// ====== NEW: nhận dữ liệu từ MQTT ======
void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  msg.trim();   // NEW

  Serial.print("Received: ");
  Serial.println(msg);

  if (msg == "OFF") {
    sendData = false;
    Serial.println("STOP sending");   // NEW
  } 
  else if (msg == "ON") {
    sendData = true;
    Serial.println("START sending");  // NEW
  }
}

// ====== CONNECT MQTT ======
void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting MQTT...");

    if (client.connect(("ESP32_HUNG" + String(random(1000, 9999))).c_str())) {
      Serial.println("connected!");

      client.subscribe("hung123/control");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again...");
      delay(2000);
    }
  }
}

// ====== SETUP ======
void setup() {
  Serial.begin(115200);

  randomSeed(analogRead(0));   // NEW

  setup_wifi();

  client.setServer(mqtt_server, 1883);

  // ====== NEW: đăng ký callback ======
  client.setCallback(callback);   // NEW
}

// ====== LOOP ======
void loop() {
  if (!client.connected()) {
    reconnect();
  }

  client.loop();

  if (sendData) {
    int value = random(0, 100);

    char msg[10];
    sprintf(msg, "%d", value);

    client.publish("hung123/test", msg);

    Serial.print("Sent: ");
    Serial.println(msg);
  }

  delay(2000);
}