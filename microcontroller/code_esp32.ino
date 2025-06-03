#include <WiFi.h>
#include <PubSubClient.h>

extern "C" {
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
}

// SSID and PASS of the synchronization network
const char* ssid = "ssid";
const char* password = "password";

// Client and Password configured for connection with Mosquito
const char* mqtt_user = "mqtt_user";
const char* mqtt_pass = "mqtt_pass";

// IP of your Mosquitto broker
const char* mqtt_server = "100.100.100.100";

const int mqtt_port = 1883;
const char* mqtt_topic = "iot/esp32/sniffer";

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastHeartbeat = 0;
const unsigned long heartbeatInterval = 30000;  // 30 seconds

// ===================
// CALLBACK PROMISCUO
// ===================
void IRAM_ATTR snifferCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* mac = ppkt->payload;
  int rssi = ppkt->rx_ctrl.rssi;

  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[10], mac[11], mac[12],
          mac[13], mac[14], mac[15]);

  char payload[100];
  sprintf(payload, "{\"mac\":\"%s\",\"rssi\":%d}", macStr, rssi);

  Serial.print("Publicado: ");
  Serial.println(payload);

  if (client.connected()) {
    client.publish(mqtt_topic, payload);
  } else {
    Serial.println("MQTT no conectado. Intentando reconectar...");
  }
}

// ===================
// MQTT RECONNECT
// ===================
void reconnectMQTT() {
  if (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    if (client.connect("ESP32Sniffer", mqtt_user, mqtt_pass)) {
      Serial.println("Conectado al broker!");
    } else {
      Serial.print("Fallo, rc=");
      Serial.println(client.state());
    }
  }
}

// ===================
// SETUP
// ===================
void setup() {
  Serial.begin(115200);

  // WiFi and NVS Initialization
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    nvs_flash_erase();
    nvs_flash_init();
  }

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  WiFi.begin(ssid, password);
  Serial.print("Conectando al WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP local: ");
  Serial.println(WiFi.localIP());

  // Configure MQTT client
  client.setServer(mqtt_server, mqtt_port);

  reconnectMQTT();  // First connection attempt

  // Initialize WiFi in promiscuous mode
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_NULL));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
  ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(&snifferCallback));

  Serial.println("Sniffer iniciado correctamente");
}

// ===================
// LOOP
// ===================
void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Intentando reconectar...");
    WiFi.begin(ssid, password);
    delay(5000);
    return;
  }

  if (!client.connected()) {
    reconnectMQTT();
  }

  client.loop();

  // Post "heartbeat" every 30 seconds
  if (millis() - lastHeartbeat > heartbeatInterval) {
    lastHeartbeat = millis();
    if (client.connected()) {
      const char* heartbeat = "{\"status\":\"alive\"}";
      client.publish(mqtt_topic, heartbeat);
      Serial.println("Publicado heartbeat.");
    }
  }

  delay(1000);
}
