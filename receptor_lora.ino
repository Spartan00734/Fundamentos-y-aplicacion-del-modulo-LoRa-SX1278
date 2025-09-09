#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// WiFi / MQTT
const char* WIFI_SSID = "Planta_alta";
const char* WIFI_PASS = "65431993";
const char* MQTT_HOST = "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "ESP32_GW_LoRa_PIPV_LATP";
const char* MQTT_TOPIC_UP = "lora/gateway/up";

// *** OJO: 433 MHz ***
const long LORA_FREQUENCY = 433E6;

// Pines ESP32 (VSPI)
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26
#define PIN_SCK    18
#define PIN_MISO   19
#define PIN_MOSI   23

const uint8_t  SPREADING_FACTOR = 9;
const long     SIGNAL_BW        = 125E3;
const uint8_t  CODING_RATE_4    = 5;

WiFiClient espClient;
PubSubClient mqtt(espClient);

void setupLoRa() {
  // Asegura pines SPI correctos en ESP32
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, LORA_SS);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println(F("Fallo LoRa.begin()"));
    while (1);
  }
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setSignalBandwidth(SIGNAL_BW);
  LoRa.setCodingRate4(CODING_RATE_4);
  LoRa.setSyncWord(0x34);   // <-- igual que el UNO
  LoRa.enableCrc();
  Serial.println(F("LoRa OK (433 MHz)"));
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print(F("WiFi conectando"));
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.print(F("\nWiFi OK, IP: "));
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  Serial.print(F("MQTT conectando"));
  while (!mqtt.connected()) {
    mqtt.connect(MQTT_CLIENT_ID);
    if (!mqtt.connected()) { Serial.print("."); delay(500); }
  }
  Serial.println(F("\nMQTT OK"));
}

void ensureConnections() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  connectMQTT();
  setupLoRa();
  Serial.println(F("ESP32 Gateway listo"));
}

void loop() {
  ensureConnections();
  mqtt.loop();

  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  String payload;
  while (LoRa.available()) payload += (char)LoRa.read();
  int rssi = LoRa.packetRssi();

  if (payload.length() == 0) return;

  StaticJsonDocument<256> inDoc;
  DeserializationError err = deserializeJson(inDoc, payload);
  if (err || !inDoc.containsKey("Equipo") || !inDoc.containsKey("Contador")) {
    Serial.print(F("Descartado (JSON invalido): "));
    Serial.println(payload);
    return;
  }

  StaticJsonDocument<256> outDoc;
  outDoc["Equipo"]   = inDoc["Equipo"];
  outDoc["Contador"] = inDoc["Contador"];
  outDoc["RSSI"]     = rssi;

  char outBuf[256];
  size_t n = serializeJson(outDoc, outBuf, sizeof(outBuf));
  bool ok = mqtt.publish(MQTT_TOPIC_UP, outBuf, n);

  Serial.print(F("LoRa->MQTT "));
  Serial.print(ok ? F("OK: ") : F("FAIL: "));
  Serial.print(outBuf);
  Serial.print(F("  (packetSize="));
  Serial.print(packetSize);
  Serial.println(F(")"));
}
