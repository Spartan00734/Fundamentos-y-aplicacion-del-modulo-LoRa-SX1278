[README_LoRa.md](https://github.com/user-attachments/files/22166378/README_LoRa.md)
# README – Fundamentos y aplicación del módulo LoRa SX1278 con Arduino UNO y ESP32

**Equipo: 4**  
**Integrantes:** Pedro Iván Palomino Viera, Luis Antonio Torres Padrón  

---

## 1. Objetivo General
Diseñar e implementar un sistema de comunicación punto a punto con módulos LoRa SX1278, donde un Arduino UNO transmita mensajes JSON con contador de pulsaciones y un ESP32 actúe como gateway, añadiendo el RSSI y publicando en un broker MQTT para su visualización web.

## 2. Objetivos Específicos
1. Configurar el módulo SX1278 en Arduino UNO para enviar datos JSON.  
2. Implementar un contador por pulsaciones con botón y pull-up interna.  
3. Desarrollar un gateway en ESP32 que reciba paquetes LoRa y mida el RSSI.  
4. Publicar los mensajes enriquecidos en formato JSON hacia un broker MQTT.  
5. Visualizar en una página web los registros con timestamp, Equipo, Contador y RSSI.  
6. Permitir exportar los datos en formato CSV para análisis y reporte.  

## 3. Competencias
- Configuración de módulos LoRa SX1278 en entornos IoT.  
- Desarrollo de comunicación punto a punto y medición de RSSI.  
- Integración LoRa con MQTT mediante WiFi.  
- Programación en Arduino (UNO y ESP32) con librerías especializadas.  
- Documentación técnica profesional con estructura README.  

## 4. Tabla de Contenidos
1. Objetivo General  
2. Objetivos Específicos  
3. Competencias  
4. Tabla de Contenidos  
5. Descripción  
6. Requisitos  
7. Instalación y Configuración  
8. Conexiones de Hardware  
9. Parámetros Técnicos del SX1278  
10. Uso y ejemplos de Código  
11. Resultados de Prueba  
12. Consideraciones Éticas y de Seguridad  
13. Formato de Salida (JSON)  
14. Solución de Problemas  
15. Contribuciones  
16. Referencias  

## 5. Descripción
Esta práctica implementa un enlace LoRa punto a punto: el Arduino UNO transmite mensajes JSON con un contador de pulsaciones, y el ESP32 recibe los paquetes, añade el valor de RSSI y los publica mediante MQTT en un broker. Una aplicación web suscrita al topic muestra los datos en tiempo real y permite exportarlos en CSV, cumpliendo los requisitos de la actividad en entornos inteligentes.

## 6. Requisitos
**Hardware necesario:**  
- Arduino UNO  
- ESP32 DevKit  
- 2 módulos SX1278 (433 MHz o 915 MHz)  
- Botón de pulsación  
- Protoboard y cables Dupont  
- Antenas para cada módulo LoRa (obligatorio)  
- Laptop con Arduino IDE  

**Software y bibliotecas requeridas:**  
- Arduino IDE actualizado  
- Librerías: LoRa.h, PubSubClient.h, ArduinoJson.h, WiFi.h  
- Broker MQTT público (ej. test.mosquitto.org) o local  
- Cliente MQTT (MQTT Explorer o web)  

**Conocimientos previos imprescindibles:**  
- Programación en Arduino (C/C++)  
- Fundamentos de IoT y protocolos MQTT  
- Conceptos básicos de radiofrecuencia y LoRa  
- Manejo de JSON  

## 7. Instalación y Configuración
1. Clonar o descargar el repositorio de la práctica.  
2. Abrir los sketches de Arduino UNO y ESP32 en Arduino IDE.  
3. Instalar las bibliotecas requeridas desde el gestor de librerías.  
4. Configurar credenciales WiFi, broker MQTT y TEAM_ID (PIPV_LATP).  
5. Conectar correctamente los módulos SX1278 según los diagramas.  
6. Cargar el código en UNO y ESP32.  
7. Ejecutar la página web `lora_dashboard.html` en un servidor local (Live Server o similar).  

## 8. Conexiones de Hardware

**Arduino UNO (Transmisor):**

| Señal          | Pin UNO        |
|----------------|----------------|
| SX1278 NSS     | D10            |
| SX1278 RST     | D9             |
| SX1278 DIO0    | D2             |
| MOSI/MISO/SCK  | D11 / D12 / D13|
| Botón          | D3 → GND       |
| Alimentación   | 3.3 V / GND    |

**ESP32 (Gateway):**

| Señal          | Pin ESP32      |
|----------------|----------------|
| SX1278 NSS     | GPIO5          |
| SX1278 RST     | GPIO14         |
| SX1278 DIO0    | GPIO26         |
| MOSI/MISO/SCK  | GPIO23/19/18   |
| Alimentación   | 3.3 V / GND    |

## 9. Parámetros Técnicos del SX1278

| Parámetro           | Valor típico | Unidad |
|---------------------|-------------|--------|
| Voltaje de operación| 3.3         | V      |
| Frecuencia          | 433 / 915   | MHz    |
| Spreading Factor    | 7–12        | -      |
| Bandwidth           | 7.8–500     | kHz    |
| Coding Rate         | 4/5–4/8     | -      |
| Potencia de salida  | 2–20        | dBm    |

## 10. Uso y ejemplos de Código
- **Arduino UNO:** cada pulsación en el botón genera un JSON con `Equipo` y `Contador`, que se envía por LoRa.  
- **ESP32:** recibe el JSON, añade `RSSI` y lo publica en MQTT en el topic configurado.  
- **Web:** se visualizan los registros en una tabla con exportación CSV.  

## 11. Resultados de Prueba
Durante las pruebas en mesa se verificó que cada pulsación genera un mensaje único con incremento correcto del contador. En el ESP32 se observa la llegada de los mensajes con su respectivo RSSI. En la aplicación web los datos aparecen en tiempo real y pueden exportarse a CSV. En pruebas de campo se midió el RSSI en distintos puntos A–E del campus, con observaciones sobre pérdida de paquetes y variación de señal.

## 12. Consideraciones Éticas y de Seguridad
- No usar los módulos sin antena para evitar daños.  
- No incluir credenciales reales de WiFi en repositorios públicos.  
- Uso educativo y responsable de la infraestructura de red.  
- Cumplir normas de espectro y no interferir en otras comunicaciones.  
- Documentar resultados con honestidad, sin alterar mediciones.  

## 13. Formato de Salida (JSON)

| Campo    | Tipo de dato | Descripción                                |
|----------|--------------|--------------------------------------------|
| Equipo   | String       | Identificador del equipo (PIPV_LATP).       |
| Contador | Entero       | Número de pulsaciones registradas.         |
| RSSI     | Entero       | Intensidad de señal recibida en dBm.       |

## 14. Solución de Problemas

| Problema         | Causa probable           | Solución                      |
|------------------|--------------------------|-------------------------------|
| No conecta LoRa  | Pines mal conectados     | Revisar cableado.             |
| No recibe ESP32  | Frecuencia distinta      | Ajustar `LORA_FREQUENCY`.     |
| MQTT no conecta  | WiFi/MQTT mal configurado| Revisar credenciales y host.  |
| JSON inválido    | Error en formato         | Verificar con ArduinoJson.    |
| RSSI incoherente | Mala antena o distancia extrema | Usar antena adecuada. |

## 15. Contribuciones
1. Realizar fork del repositorio.  
2. Crear una rama `feature/mi-mejora`.  
3. Hacer commits claros y documentados.  
4. Abrir Pull Request describiendo cambios y pruebas realizadas.  

## 16. Referencias
- Semtech Corporation. (2015). SX1278 LoRa Transceiver Datasheet.  
- Espressif Systems. ESP32 Series Datasheet.  
- Arduino. Arduino Uno Reference Design.  
- LoRa Alliance. LoRaWAN Specification.  
- Banks, A., & Gupta, R. (2014). MQTT Version 3.1.1 Specification.  
- Bray, T. (2014). JSON Data Interchange Format (RFC 7159).  
- Fette, I., & Melnikov, A. (2011). WebSocket Protocol (RFC 6455).  
