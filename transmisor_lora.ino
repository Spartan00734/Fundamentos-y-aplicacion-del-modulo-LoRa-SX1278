/***** Arduino UNO • Transmisor LoRa (SX1278)
 *  Equipo 4: Pedro Iván Palomino Viera, Luis Antonio Torres Padrón
 *  TEAM_ID: PIPV_LATP
 *  Envía JSON {Equipo, Contador} por LoRa; contador incrementa con botón (pull-up)
 *****/
#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

// --- CONFIGURA AQUÍ ---
const char* TEAM_ID = "PIPV_LATP";
const long  LORA_FREQUENCY = 433E6;  // 433E6 o 915E6 según tu módulo

// Pines SX1278 (Arduino UNO)
#define LORA_SS   10
#define LORA_RST   9
#define LORA_DIO0  2

// Botón con pull-up interna (botón a GND)
#define BTN_PIN    3

// Parámetros LoRa
const uint8_t  SPREADING_FACTOR = 9;      // 7..12
const long     SIGNAL_BW        = 125E3;  // 7.8E3..500E3
const uint8_t  CODING_RATE_4    = 5;      // 5..8 => 4/5..4/8
const int8_t   TX_POWER_DBM     = 14;     // 2..20 (PA_BOOST en algunos módulos)

uint32_t contador = 0;
unsigned long lastDebounceMs = 0;
const unsigned long DEBOUNCE_MS = 50;
bool stable = HIGH, lastStable = HIGH;

void setupLoRa() {
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) while (1);
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setSignalBandwidth(SIGNAL_BW);
  LoRa.setCodingRate4(CODING_RATE_4);
  LoRa.enableCrc();
  LoRa.setTxPower(TX_POWER_DBM);
}

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  Serial.begin(115200);
  setupLoRa();
  Serial.println(F("UNO TX listo (PIPV_LATP)"));
}

bool pressedEdge(bool raw) {
  // INPUT_PULLUP: LOW=presionado
  if (millis() - lastDebounceMs > DEBOUNCE_MS) {
    if (raw != stable) {
      stable = raw;
      lastDebounceMs = millis();
      if (lastStable == HIGH && stable == LOW) { lastStable = stable; return true; }
      lastStable = stable;
    }
  }
  return false;
}

void loop() {
  bool raw = digitalRead(BTN_PIN);
  if (pressedEdge(raw)) {
    contador++;

    StaticJsonDocument<128> doc;
    doc["Equipo"] = TEAM_ID;
    doc["Contador"] = contador;

    char payload[128];
    size_t n = serializeJson(doc, payload, sizeof(payload));

    LoRa.beginPacket();
    LoRa.write((uint8_t*)payload, n);
    LoRa.endPacket();

    Serial.print(F("Enviado: "));
    Serial.println(payload);
  }
}
