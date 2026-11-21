// Configuración del módulo A (nodo de monitoreo): pines y qué sensores están conectados.
#pragma once
#include <Arduino.h>

// ---- Sensores habilitados -------------------------------------------------
// Ponelos en 1 a medida que los vas cableando. Un sensor en 0 aparece vacío en el CSV,
// así no se confunde ruido de un pin flotante con una lectura real.
#define USE_SOIL  1   // sensor de nivel de agua clavado en la maceta
#define USE_DHT   1   // DHT11 (temperatura y humedad del aire)
#define USE_LM35  1   // LM35DZ (segunda temperatura)
#define USE_LDR   1   // fotoresistencia + 10 kΩ

// ---- Pines ----------------------------------------------------------------
constexpr uint8_t PIN_SOIL_SIGNAL = 34;  // ADC1, solo entrada
constexpr uint8_t PIN_SOIL_POWER  = 25;  // alimenta el sensor solo mientras mide
constexpr uint8_t PIN_DHT         = 4;
constexpr uint8_t PIN_LM35        = 36;  // "VP" en la placa, ADC1
constexpr uint8_t PIN_LDR         = 39;  // "VN" en la placa, ADC1
constexpr uint8_t PIN_BUTTON      = 23;  // a GND, con pull-up interno
constexpr uint8_t PIN_LED_BUILTIN = 2;   // LED azul: parpadea en cada lectura

// ---- Tiempos y filtros ----------------------------------------------------
constexpr uint32_t SAMPLE_PERIOD_MS = 10000;  // una fila de CSV cada 10 s
constexpr uint32_t SOIL_SETTLE_MS   = 50;     // espera tras energizar el sensor
constexpr size_t   ADC_SAMPLES      = 16;     // muestras por lectura
constexpr size_t   ADC_TRIM         = 3;      // descartadas en cada extremo
constexpr float    EMA_ALPHA        = 0.2f;
