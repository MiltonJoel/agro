// Filtros compartidos entre monitor y spray.
#pragma once
#include <Arduino.h>
#include <math.h>

// Media recortada: ordena n muestras, descarta `trim` de cada extremo y promedia el resto.
// Sirve para sacar picos de ruido del ADC del ESP32 sin la lentitud de una mediana larga.
inline float trimmedMean(uint32_t *v, size_t n, size_t trim) {
  // Ordenamiento por inserción: n es chico (16), no vale la pena nada más complejo.
  for (size_t i = 1; i < n; i++) {
    uint32_t x = v[i];
    size_t j = i;
    while (j > 0 && v[j - 1] > x) { v[j] = v[j - 1]; j--; }
    v[j] = x;
  }
  if (2 * trim >= n) trim = 0;
  uint64_t sum = 0;
  for (size_t i = trim; i < n - trim; i++) sum += v[i];
  return (float)sum / (float)(n - 2 * trim);
}

// Lee `n` muestras en milivoltios (ADC calibrado de fábrica) y devuelve la media recortada.
inline float readMilliVoltsFiltered(uint8_t pin, size_t n = 16, size_t trim = 3) {
  uint32_t buf[32];
  if (n > 32) n = 32;
  for (size_t i = 0; i < n; i++) {
    buf[i] = analogReadMilliVolts(pin);
    delayMicroseconds(200);
  }
  return trimmedMean(buf, n, trim);
}

// Filtro de media móvil exponencial: y = a*x + (1-a)*y_prev.
// Con NAN como estado inicial, la primera muestra se toma tal cual.
struct Ema {
  float alpha;
  float value = NAN;
  explicit Ema(float a) : alpha(a) {}
  float update(float x) {
    if (isnan(x)) return value;          // una lectura fallida no contamina el filtro
    value = isnan(value) ? x : alpha * x + (1.0f - alpha) * value;
    return value;
  }
};
