# Agro project

Nodo agro de precisión con un kit ESP32: monitoreo agroclimático (módulo A) y demo de pulverización selectiva (módulo B).

## Estructura

```
firmware/monitor/   Módulo A: suelo, T/HR, luz, VPD, alertas
firmware/spray/     Módulo B: sensor de color ExG + servo
lib/common/         Código compartido (filtros, Wi-Fi, Telegram)
include/            secrets.h (no versionado) y secrets.example.h
docs/               Diagramas y fotos
data/               CSV de mediciones
```

## Primeros pasos

```bash
cp include/secrets.example.h include/secrets.h   # completar credenciales
pio run -e monitor -t upload -t monitor          # o -e spray
```

Deberías ver el LED azul parpadeando y `hola desde el modulo monitor` en el monitor serie (115200 baudios).

## Fase 1: sensado del nodo (módulo A)

Sensores habilitados en `firmware/monitor/config.h` (`USE_SOIL`, `USE_DHT`, `USE_LM35`, `USE_LDR`). El firmware imprime una fila CSV cada 10 s:

```
ms,soil_mv,soil_ema_mv,soil_pct,t_dht_c,hr_dht,t_lm35_c,luz_mv,luz_pct
```

- **Suelo:** el sensor se energiza por GPIO25 solo al medir (50 ms de espera, 16 muestras, media recortada, EMA α = 0.2).
- **Calibración de dos puntos:** pulsador en GPIO23 (o tecla `c` en el monitor serie): tierra seca → botón → tierra regada → botón. Se guarda en la flash. `p` la muestra, `b` la borra.
- **Registro en la PC:** `~/.platformio/penv/bin/python tools/log_serial.py` guarda `data/fase1_*.csv` con la hora; `python3 tools/plot_fase1.py data/<archivo>.csv` genera el gráfico crudo vs EMA.
