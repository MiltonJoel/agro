#!/usr/bin/env python3
"""Grafica un CSV de la Fase 1: suelo crudo vs filtrado (EMA), temperaturas y luz.

Necesita pandas y matplotlib:  pip install pandas matplotlib
Uso:  python3 tools/plot_fase1.py data/fase1_AAAAMMDD_HHMM.csv
Guarda el PNG junto al CSV (sirve para docs/ y el README).
"""
import sys

import matplotlib.pyplot as plt
import pandas as pd

path = sys.argv[1]
df = pd.read_csv(path, parse_dates=["hora"])

panels = [("Suelo (mV)", ["soil_mv", "soil_ema_mv"], ["crudo", "EMA"]),
          ("Temperatura (°C)", ["t_dht_c", "t_lm35_c"], ["DHT11", "LM35"]),
          ("HR aire (%)", ["hr_dht"], ["DHT11"]),
          ("Luz (%)", ["luz_pct"], ["fotoresistencia"])]
panels = [p for p in panels if df[p[1]].notna().any().any()]

fig, axes = plt.subplots(len(panels), 1, sharex=True, figsize=(10, 2.6 * len(panels)), squeeze=False)
for ax, (title, cols, labels) in zip(axes[:, 0], panels):
    for col, label in zip(cols, labels):
        if df[col].notna().any():
            ax.plot(df["hora"], df[col], label=label, lw=1 if col == "soil_mv" else 1.6,
                    alpha=0.5 if col == "soil_mv" else 1)
    ax.set_ylabel(title)
    ax.grid(alpha=0.3)
    ax.legend(loc="upper right")
fig.autofmt_xdate()
fig.tight_layout()
out = path.rsplit(".", 1)[0] + ".png"
fig.savefig(out, dpi=150)
print(f"Gráfico guardado en {out}")
