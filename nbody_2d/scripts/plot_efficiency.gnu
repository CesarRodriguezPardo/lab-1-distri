# plot_efficiency.gnu
# ─────────────────────────────────────────────────────────────────────────────
# Visualización de Eficiencia paralela para el sistema N-Body.
#
# ENTRADA:  scaling_parallel.dat
#   Columnas: Threads AvgTime StdDevTime Speedup SpeedupError
#             Efficiency EfficiencyError SerialFraction
#   Primera fila: encabezado → se salta con 'every ::1'
#
# SALIDA:   efficiency_plot.png
#   Panel superior: E_p = S_p / p con barras de error ± σ_Ep
#                   + línea ideal E = 1
#                   + predicción de Amdahl
#   Panel inferior: Tiempo promedio T_p vs. número de hilos con barras ± σ_Tp
#
# USO:
#   gnuplot scripts/plot_efficiency.gnu    ← directo
#   make plot                              ← vía Makefile (recomendado)
# ─────────────────────────────────────────────────────────────────────────────

DAT = 'scaling_parallel.dat'

# ── Detectar pmax ─────────────────────────────────────────────────────────────
pmax = int(system("awk 'NR>1{c++} END{print c}' ".DAT))
if (pmax == 0) {
    print "ERROR: ".DAT." no encontrado o vacío. Ejecute primero: make benchmark"
    exit status 1
}

# ── Fracción serial promedio (misma lógica que en plot_speedup.gnu) ───────────
f_sum   = 0.0
f_count = 0
do for [row=2:pmax] {
    f_val = real(system("awk 'NR==".( row+1 )." {print $8}' ".DAT))
    if (f_val >= 0.0 && f_val <= 1.0) {
        f_sum   = f_sum + f_val
        f_count = f_count + 1
    }
}
f_serial = (f_count > 0 ? f_sum / f_count : 0.1)

# Eficiencia de Amdahl: E_p = S_p / p = 1 / (p·f + (1-f))
amdahl_eff(p) = 1.0 / (p * f_serial + (1.0 - f_serial))

# ── Terminal de salida ────────────────────────────────────────────────────────
set terminal pngcairo size 1000,800 enhanced font 'Helvetica,10' background '#FFFFFF'
set output 'efficiency_plot.png'

set datafile separator whitespace

# ── Estética global ───────────────────────────────────────────────────────────
set grid linestyle 1 lc rgb '#CCCCCC' lw 0.5 dt 2
set lmargin 14
set rmargin 8
set style line 1 lc rgb '#E07B54' lw 2 pt 7 ps 1.2   # medido
set style line 2 lc rgb '#27AE60' lw 1.5 dt 2         # ideal (E=1)
set style line 3 lc rgb '#4A90D9' lw 1.5 dt 4         # Amdahl
set style line 4 lc rgb '#9B59B6' lw 2.0 pt 9 ps 1.2  # tiempo

# ── Multiplot: 2 paneles ──────────────────────────────────────────────────────
set multiplot layout 2,1 \
    title "Eficiencia y Tiempo de Ejecución — Sistema N-Body" \
    font ",13"

# ═══════════════════════════════════════════════════════════════════════════════
# PANEL 1: Eficiencia E_p vs. número de hilos
# ═══════════════════════════════════════════════════════════════════════════════
set title "Eficiencia E_p = S_p / p" font ",11" offset 0,-0.3
set ylabel "Eficiencia E_p" font ",10"
unset xlabel
set bmargin 0
set tmargin 2
set xrange [0.5 : pmax + 0.5]
set yrange [0 : 1.3]
set xtics 1

set key top right font ",9" spacing 1.3 box lc rgb '#AAAAAA'

# Región sombreada "eficiencia aceptable" (E ≥ 0.7)
set object 1 rect from 0.5,0.7 to pmax+0.5,1.3 \
    fc rgb '#27AE60' fillstyle solid 0.06 noborder

set label 1 "zona eficiente (E ≥ 0.7)" at 0.6, 1.25 \
    font ",8" tc rgb '#27AE60'

plot 1 with lines ls 2 title 'Ideal (E = 1)', \
     [1:pmax] amdahl_eff(x) \
         with lines ls 3 title sprintf('Amdahl (f=%.3f)', f_serial), \
     DAT every ::1 using 1:6:7 \
         with yerrorbars ls 1 title 'Medido (±σ)', \
     DAT every ::1 using 1:6 \
         with linespoints ls 1 notitle

unset object 1
unset label 1

# ═══════════════════════════════════════════════════════════════════════════════
# PANEL 2: Tiempo promedio T_p vs. número de hilos (escala lineal)
# ═══════════════════════════════════════════════════════════════════════════════
set title "Tiempo de ejecución T_p (con barras de error ±σ)" \
    font ",11" offset 0,-0.3
set ylabel "T_p [s]" font ",10"
set xlabel "Número de hilos p" font ",10"
set bmargin 3
set tmargin 0
set yrange [0 : *]
unset key

plot DAT every ::1 using 1:2:3 \
         with yerrorbars ls 4 title 'T_p medido', \
     DAT every ::1 using 1:2 \
         with linespoints ls 4 notitle

unset multiplot
print "efficiency_plot.png generado correctamente."
