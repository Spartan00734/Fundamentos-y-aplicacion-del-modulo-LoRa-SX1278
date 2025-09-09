/**
 * UNO_Transmisor_LoRa.ino
 *
 * Descripción:
 *   Transmisor LoRa con botón (pull-up). Cada pulsación envía json:
 *   {"Equipo":"PIPV_LATP","Contador":N}
 *
 * Equipo / Práctica:
 *   Equipo 4 - PIPV_LATP (Pedro Iván Palomino Viera, Luis Antonio Torres Padrón)
 *
 * Botón:
 *   D3 como INPUT_PULLUP (botón a GND). LOW = presionado.
 */

#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

/*=============================
=      CONFIGURACIÓN          =
=============================*/

// Identidad para la práctica
const char* TEAM_ID = "PIPV_LATP";

// LoRa (mismo set en el gateway)
const long  LORA_FREQUENCY     = 433E6;   // 433E6 o 915E6 según módulo
const uint8_t  SPREADING_FACTOR = 9;
const long     SIGNAL_BW        = 125E3;
const uint8_t  CODING_RATE_4    = 5;
const uint8_t  SYNC_WORD        = 0x34;
const bool     USE_CRC          = true;

// Potencia de TX (dBm) — bajar en pruebas de mesa para evitar saturación/picos
const int8_t   TX_POWER_DBM     = 14;     // 2..20 (según módulo/PA)

// Pines SX1278 (Arduino UNO)
#define LORA_SS    10
#define LORA_RST    9
#define LORA_DIO0   2

// Botón (pull-up interna; botón a GND)
#define BTN_PIN     3

// Antirrebote (ms)
const unsigned long DEBOUNCE_MS = 50;

/*=====  End of CONFIGURACIÓN  =====*/

// Estado de botón/contador
uint32_t contador = 0;
unsigned long lastDebounceMs = 0;
bool stable = HIGH, lastStable = HIGH;

/**
 * @brief Inicializa el radio LoRa con los parámetros seleccionados.
 */
void setupLoRa() {
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    // Si falla aquí, revisar cableado, 3.3 V y antena
    while (1) { }
  }
  LoRa.setSpreadingFactor(SPREADING_FACTOR);
  LoRa.setSignalBandwidth(SIGNAL_BW);
  LoRa.setCodingRate4(CODING_RATE_4);
  LoRa.setSyncWord(SYNC_WORD);
  if (USE_CRC) LoRa.enableCrc(); else LoRa.disableCrc();
  LoRa.setTxPower(TX_POWER_DBM);
}

/**
 * @brief Setup general: botón con pull-up, Serial para logs, y radio LoRa.
 */
void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);  // HIGH en reposo, LOW presionado
  Serial.begin(115200);
  setupLoRa();
  Serial.println(F("[BOOT] UNO TX listo (PIPV_LATP)"));
}

/**
 * @brief Antirrebote por flanco: devuelve true solo cuando hay
 *        transición estable de HIGH->LOW (pulsación real).
 */
bool pressedEdge(bool raw) {
  // raw = digitalRead(BTN_PIN);  // HIGH reposo / LOW presionado
  if (millis() - lastDebounceMs > DEBOUNCE_MS) {
    if (raw != stable) {
      stable = raw;
      lastDebounceMs = millis();
      if (lastStable == HIGH && stable == LOW) { // flanco de bajada
        lastStable = stable;
        return true;
      }
      lastStable = stable;
    }
  }
  return false;
}

void loop() {
  // 1) Lee botón y detecta flanco con antirrebote
  bool raw = digitalRead(BTN_PIN);
  if (pressedEdge(raw)) {
    // 2) Incrementa contador por pulsación válida
    contador++;

    // 3) Construye JSON {"Equipo","Contador"} usando memoria estática (UNO=2KB RAM)
    StaticJsonDocument<128> doc;
    doc["Equipo"]   = TEAM_ID;
    doc["Contador"] = contador;

    char payload[128];
    size_t n = serializeJson(doc, payload, sizeof(payload));

    // 4) Transmite paquete LoRa (bloqueo breve durante el envío)
    LoRa.beginPacket();
    LoRa.write((uint8_t*)payload, n);
    LoRa.endPacket();

    // 5) Log por Serial (útil para evidencias)
    Serial.print(F("[TX] Enviado: "));
    Serial.println(payload);
  }
}

