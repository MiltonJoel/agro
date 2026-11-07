// Paso 0: blink + "hola" por serial para comprobar placa, driver y toolchain.
#include <Arduino.h>
#include "common.h"

#if __has_include("secrets.h")
#include "secrets.h"
#else
#warning "Falta include/secrets.h: copiá include/secrets.example.h y completalo."
#endif

constexpr uint8_t LED_PIN = 2;  // LED azul integrado de la NodeMCU-32S
constexpr uint32_t BLINK_MS = 500;

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(LED_PIN, OUTPUT);
  Serial.printf("hola desde el modulo %s (agro v%s)\n", MODULE_NAME, AGRO_VERSION);
}

void loop() {
  static uint32_t last = 0;
  static bool on = false;
  if (millis() - last >= BLINK_MS) {  // no bloqueante, como el resto del plan
    last = millis();
    on = !on;
    digitalWrite(LED_PIN, on);
    Serial.printf("[%lu ms] led=%d\n", (unsigned long)millis(), on);
  }
}
