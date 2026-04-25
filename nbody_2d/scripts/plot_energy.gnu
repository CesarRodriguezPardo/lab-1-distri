# plot_energy.gnu
# ─────────────────────────────────────────────────────────────────────────────
# Visualización de la energía del sistema N-Body a lo largo del tiempo.
#
# ENTRADA:  energy_timeseries.dat
#   Formato: K_Cinetica  U_Potencial  E_Total  (separado por whitespace)
#   Primera fila: encabezado → se salta con 'every ::1'
#
# SALIDA:   energy_plot.png
#   Tres paneles apilados: K(t), U(t), E = K + U (t)
#
# USO:
#   gnuplot scripts/plot_energy.gnu    ← directo
#   make plot                          ← vía Makefile (recomendado)
#
# INSTALACIÓN DE GNUPLOT:
#   macOS → brew install gnuplot
#   Debian/Ubuntu → apt-get install gnuplot
# ─────────────────────────────────────────────────────────────────────────────

# ── Terminal de salida ────────────────────────────────────────────────────────
# pngcairo: PNG de alta resolución con antialiasing (mejor que el terminal 'png')
# enhanced: permite notación especial ({/*0.8 texto}, superíndices, etc.)
set terminal pngcairo size 1100,900 enhanced font 'Helvetica,10' background '#FFFFFF'
set output 'energy_plot.png'

# El archivo usa espacios y tabulaciones como separadores
set datafile separator whitespace

# ── Estética global ───────────────────────────────────────────────────────────
set grid linestyle 1 lc rgb '#CCCCCC' lw 0.5 dt 2
unset key     # sin leyenda (los títulos de panel ya describen la serie)

# Márgenes fijos: garantizan que los tres paneles queden alineados verticalmente
# aunque las etiquetas del eje Y tengan distinto ancho.
set lmargin 12
set rmargin 4

# ── Multiplot: 3 paneles apilados ─────────────────────────────────────────────
# Los tres comparten el mismo eje X (paso de simulación).
# 'rowsfirst' apila de arriba hacia abajo (comportamiento por defecto en layout N,1).
set multiplot layout 3,1 \
    title "Energía del sistema N-Body — Integrador de Euler explícito\n" \
    font ",13"

# ── PANEL 1: K(t) — Energía cinética ─────────────────────────────────────────
# K = (1/2) * Σ m_i * (vx_i² + vy_i²)
# Siempre ≥ 0. Con Euler explícito tiende a crecer con el tiempo.
#
# 'every ::1' → salta el registro 0 (encabezado) y empieza desde el registro 1
# 'using 0:1' → eje X = índice de punto (paso), eje Y = columna 1 (K)
set title "K — Energía cinética" font ",10" offset 0,-0.3
set ylabel "K [u.a.]" font ",9"
unset xlabel
set bmargin 0    # sin espacio inferior: el panel 2 se pega inmediatamente abajo
set tmargin 2
plot 'energy_timeseries.dat' every ::1 using 0:1 \
    with lines lc rgb '#E07B54' lw 1.3 notitle

# ── PANEL 2: U(t) — Energía potencial ────────────────────────────────────────
# U = -G * Σ_{i<j} m_i * m_j / r_ij    (con suavizado eps)
# Siempre ≤ 0. Se vuelve más negativa cuando los cuerpos se aproximan.
set title "U — Energía potencial" font ",10" offset 0,-0.3
set ylabel "U [u.a.]" font ",9"
unset xlabel
set bmargin 0
set tmargin 0
plot 'energy_timeseries.dat' every ::1 using 0:2 \
    with lines lc rgb '#4A90D9' lw 1.3 notitle

# ── PANEL 3: E(t) = K + U — Energía total ────────────────────────────────────
# El integrador de Euler explícito NO conserva la energía de forma exacta
# (no es un método simpléctico: no preserva el volumen en el espacio de fases).
# Se espera una DERIVA positiva proporcional a dt.
# Para conservación exacta se necesitaría Störmer-Verlet (Leapfrog).
set title "E = K + U — Energía total  {/*0.85 (deriva esperada con Euler)}" \
    font ",10" offset 0,-0.3
set ylabel "E [u.a.]" font ",9"
set xlabel "Paso de simulación" font ",10"
set bmargin 3
set tmargin 0
plot 'energy_timeseries.dat' every ::1 using 0:3 \
    with lines lc rgb '#27AE60' lw 1.3 notitle

unset multiplot
print "energy_plot.png generado correctamente."
