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
