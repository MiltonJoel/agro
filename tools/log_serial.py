#!/usr/bin/env python3
"""Guarda el CSV que imprime el ESP32 en data/, agregando la hora de la PC.

Uso (con el python de PlatformIO, que ya trae pyserial):
    ~/.platformio/penv/bin/python tools/log_serial.py            # /dev/ttyUSB0
    ~/.platformio/penv/bin/python tools/log_serial.py COM3        # Windows

Las líneas que empiezan con '#' son mensajes del firmware: se muestran pero no se guardan.
Cortá con Ctrl+C. No dejes abierto `pio device monitor` al mismo tiempo (el puerto es uno solo).
"""
import datetime as dt
import pathlib
import sys

import serial

port = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
data_dir = pathlib.Path(__file__).resolve().parent.parent / "data"
out = data_dir / f"fase1_{dt.datetime.now():%Y%m%d_%H%M}.csv"

ser = serial.Serial()
ser.port, ser.baudrate, ser.timeout = port, 115200, 15
ser.dtr = ser.rts = False  # no resetear ni dejar la placa en modo bootloader al abrir
ser.open()

with ser, open(out, "w", buffering=1) as f:
    print(f"Guardando en {out}  (Ctrl+C para terminar)")
    header_written = False
    try:
        while True:
            line = ser.readline().decode(errors="replace").strip()
            if not line:
                continue
            if line.startswith("#"):
                print(line)
                continue
            if line.startswith("ms,"):
                if not header_written:
                    f.write("hora," + line + "\n")
                    header_written = True
                continue
            if not header_written or not line[0].isdigit():
                continue  # basura de arranque antes del encabezado
            f.write(f"{dt.datetime.now():%Y-%m-%d %H:%M:%S}," + line + "\n")
            print(line)
    except KeyboardInterrupt:
        print(f"\nListo: {out}")
