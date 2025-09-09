/**
 * ESP32_Gateway_LoRa_MQTT.ino
 * 
 * Descripción:
 *   Actúa como gateway: recibe paquetes LoRa (SX1278/RA-02), añade RSSI
 *   y publica el resultado en un broker MQTT para su visualización web.
 *
 * Flujo:
 *   LoRa RX -> valida JSON entrante {"Equipo","Contador"}
 *           -> out: {"Equipo","Contador","RSSI"}
 *           -> MQTT publish en topic configurado
 *
 * Equipo / Práctica:
 *   Equipo 4 - PIPV_LATP (Pedro Iván Palomino Viera, Luis Antonio Torres Padrón)
 *
 *
 * Notas:
 *   - Mantener los parámetros LoRa IGUALES en TX y RX (frecuencia, SF, BW, CR, SyncWord, CRC).
 *   - Antena SIEMPRE conectada antes de transmitir (evita daño).
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

/*=============================
=      CONFIGURACIÓN          =
=============================*/

// --- WiFi / MQTT (edita estas credenciales) ---
const char* WIFI_SSID = "Tu SSID";
const char* WIFI_PASS = "Tu Contraseña";
const char* MQTT_HOST = "test.mosquitto.org";
const uint16_t MQTT_PORT = 1883;                 // TCP sin TLS para ESP32
const char* MQTT_CLIENT_ID = "ESP32_GW_LoRa_PIPV_LATP";
const char* MQTT_TOPIC_UP  = "lora/gateway/up";  // Topic de salida (web se suscribe aquí)

// --- LoRa (mismo set en el transmisor) ---
const long  LORA_FREQUENCY     = 433E6;   // RA-02 que muestra "ISM 410-525 MHz"
const uint8_t  SPREADING_FACTOR = 9;      // 7..12 (más alto = más robusto/menos bitrate)
const long     SIGNAL_BW        = 125E3;  // 7.8E3..500E3 (más bajo = +sensibilidad)
const uint8_t  CODING_RATE_4    = 5;      // 5..8 => 4/5..4/8 (más alto = +corrección)
const uint8_t  SYNC_WORD        = 0x34;   // Debe coincidir con TX
const bool     USE_CRC          = true;   // true en ambos extremos o false en ambos

// --- Pines ESP32 (VSPI) ---
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26
#define PIN_SCK    18
#define PIN_MISO   19
#define PIN_MOSI   23

/*=====  End of CONFIGURACIÓN  =====*/

/* Objetos de red/MQTT */
WiFiClient espClient;
PubSubClient mqtt(espClient);

/**
 * @brief Inicializa el transceptor LoRa con los parámetros deseados.
 *        Se asegura de usar VSPI en ESP32 y configura SF/BW/CR/Sync/CRC.
 */
void setupLoRa() {
  // Iniciar bus SPI en los pines correctos (VSPI)
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println(F("[LoRa] ERROR: LoRa.begin()"));
    while (1) { delay(10); } // Falla dura si no hay radio
  }

  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setSignalBandwidth(SIGNAL_BW);
  LoRa.setCodingRate4(CODING_RATE_4);
  LoRa.setSyncWord(SYNC_WORD);
  if (USE_CRC) LoRa.enableCrc(); else LoRa.disableCrc();

  // Opcional para protoboard: reducir frecuencia SPI de LoRa (más tolerante)
  // LoRa.setSPIFrequency(4E6);

  Serial.println(F("[LoRa] OK @433 MHz"));
}

/**
 * @brief Conecta a WiFi (bloqueante simple con feedback por serial).
 */
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print(F("[WiFi] Conectando"));
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.print(F("\n[WiFi] OK, IP: "));
  Serial.println(WiFi.localIP());
}

/**
 * @brief Conecta a MQTT y deja configurado el servidor/puerto.
 */
void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  Serial.print(F("[MQTT] Conectando"));
  while (!mqtt.connected()) {
    mqtt.connect(MQTT_CLIENT_ID);
    if (!mqtt.connected()) { Serial.print("."); delay(500); }
  }
  Serial.println(F("\n[MQTT] OK"));
}

/**
 * @brief Reasegura conectividad (WiFi y MQTT) en tiempo de ejecución.
 */
void ensureConnections() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  connectMQTT();
  setupLoRa();
  Serial.println(F("[BOOT] ESP32 Gateway listo"));
}

void loop() {
  ensureConnections();   // Reconexión básica si algo cae
  mqtt.loop();           // Mantiene pings/keepalive

  // 1) ¿Llegó paquete LoRa?
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  // 2) Leer payload crudo del paquete
  String payload;
  while (LoRa.available()) payload += (char)LoRa.read();

  // 3) Métricas de recepción (útil para diagnóstico/campo)
  int   rssi = LoRa.packetRssi();
  // float snr  = LoRa.packetSnr(); // Si quieres añadirlo al JSON

  if (payload.length() == 0) return; // Paquete vacío (raro), se descarta

  // 4) Validar JSON entrante
  StaticJsonDocument<256> inDoc;
  DeserializationError err = deserializeJson(inDoc, payload);
  if (err || !inDoc.containsKey("Equipo") || !inDoc.containsKey("Contador")) {
    Serial.print(F("[LoRa] Descartado (JSON inválido): "));
    Serial.println(payload);
    return;
  }

  // 5) Armar JSON de salida (agregando RSSI)
  StaticJsonDocument<256> outDoc;
  outDoc["Equipo"]   = inDoc["Equipo"];     // p. ej. "PIPV_LATP"
  outDoc["Contador"] = inDoc["Contador"];   // contador que venía del UNO
  outDoc["RSSI"]     = rssi;                // RSSI del paquete recibido (dBm)
  // outDoc["SNR"]   = snr;                 // opcional

  char outBuf[256];
  size_t n = serializeJson(outDoc, outBuf, sizeof(outBuf));

  // 6) Publicar a MQTT
  bool ok = mqtt.publish(MQTT_TOPIC_UP, outBuf, n);

  // 7) Log de depuración
  Serial.print(F("[GW] LoRa->MQTT "));
  Serial.print(ok ? F("OK: ") : F("FAIL: "));
  Serial.print(outBuf);
  Serial.print(F("  (len="));
  Serial.print(packetSize);
  Serial.println(F(")"));
}

