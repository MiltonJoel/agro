// Módulo A, Fase 1: sensado del nodo.
// Lee humedad de suelo (sensor resistivo energizado solo al medir), DHT11, LM35 y luz,
// y escribe una fila de CSV por serial cada 10 s. Las líneas que empiezan con '#'
// son mensajes, no datos: el script tools/log_serial.py las ignora.
//
// Calibración del suelo (dos puntos, se guarda en la flash con Preferences):
//   botón (GPIO23) o tecla 'c' -> paso 1: sensor en tierra seca -> botón
//                              -> paso 2: tierra recién regada  -> botón -> guardado
// Otras teclas por serial: 'p' muestra la calibración, 'b' la borra, 'q' cancela.
#include <Arduino.h>
#include <Preferences.h>
#include "common.h"
#include "filters.h"
#include "config.h"

#if USE_DHT
#include <DHT.h>
DHT dht(PIN_DHT, DHT11);
#endif

// ---- Estado ---------------------------------------------------------------
Preferences prefs;
Ema soilEma(EMA_ALPHA);
Ema lm35Ema(EMA_ALPHA);  // el LM35 leído por el ADC del ESP32 salta ±2-3 °C entre lecturas

struct SoilCal {
  float dryMv = NAN;  // lectura con tierra seca  -> 0 %
  float wetMv = NAN;  // lectura recién regada    -> 100 %
  bool valid() const { return !isnan(dryMv) && !isnan(wetMv) && fabsf(wetMv - dryMv) > 1.0f; }
} cal;

enum class CalStep { Idle, WaitDry, WaitWet };
CalStep calStep = CalStep::Idle;

// ---- Sensores -------------------------------------------------------------
// Energiza el sensor, espera que se estabilice, mide y lo apaga.
// Tenerlo apagado el resto del tiempo frena la corrosión de las pistas por electrólisis.
float readSoilMv() {
  digitalWrite(PIN_SOIL_POWER, HIGH);
  delay(SOIL_SETTLE_MS);
  float mv = readMilliVoltsFiltered(PIN_SOIL_SIGNAL, ADC_SAMPLES, ADC_TRIM);
  digitalWrite(PIN_SOIL_POWER, LOW);
  return mv;
}

float soilPercent(float mv) {
  if (!cal.valid() || isnan(mv)) return NAN;
  float pct = (mv - cal.dryMv) / (cal.wetMv - cal.dryMv) * 100.0f;  // vale si wet < dry también
  return constrain(pct, 0.0f, 100.0f);
}

// LM35: 10 mV por °C. Con atenuación 0 dB el ADC mide ~0-950 mV con más resolución.
float readLm35C() {
  return readMilliVoltsFiltered(PIN_LM35, 32, 8) / 10.0f;  // más muestras: su señal es ruidosa
}

// ---- Calibración ----------------------------------------------------------
void loadCal() {
  prefs.begin("soil", true);
  cal.dryMv = prefs.getFloat("dry", NAN);
  cal.wetMv = prefs.getFloat("wet", NAN);
  prefs.end();
}

void saveCal() {
  prefs.begin("soil", false);
  prefs.putFloat("dry", cal.dryMv);
  prefs.putFloat("wet", cal.wetMv);
  prefs.end();
}

void printCal() {
  if (cal.valid())
    Serial.printf("# CAL seco=%.0f mV  mojado=%.0f mV\n", cal.dryMv, cal.wetMv);
  else
    Serial.println("# CAL sin calibrar (soil_pct queda vacio). Presiona el boton para calibrar.");
}

// Promedia varias lecturas para que el punto de calibración sea estable.
float captureCalPoint() {
  float sum = 0;
  for (int i = 0; i < 5; i++) { sum += readSoilMv(); delay(200); }
  return sum / 5;
}

void calAdvance() {
  switch (calStep) {
    case CalStep::Idle:
      Serial.println("# CAL paso 1/2: clava el sensor en tierra SECA y presiona el boton");
      calStep = CalStep::WaitDry;
      break;
    case CalStep::WaitDry:
      cal.dryMv = captureCalPoint();
      Serial.printf("# CAL seco = %.0f mV\n", cal.dryMv);
      Serial.println("# CAL paso 2/2: regá la maceta (o usa tierra recien regada) y presiona el boton");
      calStep = CalStep::WaitWet;
      break;
    case CalStep::WaitWet: {
      float wet = captureCalPoint();
      Serial.printf("# CAL mojado = %.0f mV\n", wet);
      if (fabsf(wet - cal.dryMv) < 200) {
        Serial.println("# CAL ERROR: seco y mojado casi iguales (<200 mV). Revisa el cableado y repeti.");
        loadCal();  // vuelve a la calibración anterior
      } else {
        cal.wetMv = wet;
        saveCal();
        Serial.println("# CAL guardada en la flash");
      }
      calStep = CalStep::Idle;
      printCal();
      break;
    }
  }
}

// Detecta el flanco de bajada del pulsador con antirrebote de 30 ms.
bool buttonPressed() {
  static bool stable = HIGH, lastRaw = HIGH;
  static uint32_t changedAt = 0;
  bool raw = digitalRead(PIN_BUTTON);
  if (raw != lastRaw) { lastRaw = raw; changedAt = millis(); }
  if (millis() - changedAt > 30 && raw != stable) {
    stable = raw;
    return stable == LOW;
  }
  return false;
}

void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case 'c': calAdvance(); break;
      case 'p': printCal(); break;
      case 'q':
        if (calStep != CalStep::Idle) { loadCal(); calStep = CalStep::Idle; Serial.println("# CAL cancelada"); }
        break;
      case 'b':
        prefs.begin("soil", false); prefs.clear(); prefs.end();
        loadCal(); Serial.println("# CAL borrada");
        break;
    }
  }
}

// ---- CSV ------------------------------------------------------------------
// Imprime un valor o deja la celda vacía si es NAN (sensor deshabilitado o con falla).
void printField(float v, int decimals) {
  Serial.print(',');
  if (!isnan(v)) Serial.print(v, decimals);
}

void sampleAndPrint() {
  float soilMv = NAN, soilEmaMv = NAN, soilPct = NAN;
  float tDht = NAN, hrDht = NAN, tLm35 = NAN, luzMv = NAN, luzPct = NAN;

#if USE_SOIL
  soilMv = readSoilMv();
  soilEmaMv = soilEma.update(soilMv);
  soilPct = soilPercent(soilEmaMv);
#endif
#if USE_DHT
  tDht = dht.readTemperature();   // NAN si el DHT11 no responde
  hrDht = dht.readHumidity();
#endif
#if USE_LM35
  tLm35 = lm35Ema.update(readLm35C());
#endif
#if USE_LDR
  luzMv = readMilliVoltsFiltered(PIN_LDR, ADC_SAMPLES, ADC_TRIM);
  luzPct = constrain(luzMv / 3300.0f * 100.0f, 0.0f, 100.0f);
#endif

  Serial.print(millis());
  printField(soilMv, 0);
  printField(soilEmaMv, 0);
  printField(soilPct, 1);
  printField(tDht, 1);
  printField(hrDht, 0);
  printField(tLm35, 1);
  printField(luzMv, 0);
  printField(luzPct, 1);
  Serial.println();
}

// ---- Arduino --------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(PIN_LED_BUILTIN, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  pinMode(PIN_SOIL_POWER, OUTPUT);
  digitalWrite(PIN_SOIL_POWER, LOW);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_SOIL_SIGNAL, ADC_11db);  // 0-3.1 V
  analogSetPinAttenuation(PIN_LDR, ADC_11db);
  analogSetPinAttenuation(PIN_LM35, ADC_0db);          // 0-0.95 V: suficiente hasta ~90 °C

#if USE_DHT
  dht.begin();
#endif

  loadCal();
  Serial.printf("# modulo %s agro v%s - Fase 1 sensado\n", MODULE_NAME, AGRO_VERSION);
  Serial.printf("# sensores: suelo=%d dht11=%d lm35=%d luz=%d\n", USE_SOIL, USE_DHT, USE_LM35, USE_LDR);
  printCal();
  Serial.println("ms,soil_mv,soil_ema_mv,soil_pct,t_dht_c,hr_dht,t_lm35_c,luz_mv,luz_pct");
}

void loop() {
  static uint32_t lastSample = 0;
  static bool first = true;

  handleSerial();
  if (buttonPressed()) calAdvance();

  // No mezclar lecturas periódicas con la calibración en curso.
  if (calStep == CalStep::Idle && (first || millis() - lastSample >= SAMPLE_PERIOD_MS)) {
    first = false;
    lastSample = millis();
    digitalWrite(PIN_LED_BUILTIN, HIGH);
    sampleAndPrint();
    digitalWrite(PIN_LED_BUILTIN, LOW);
  }
}
