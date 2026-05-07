# plot_speedup.gnu
# ─────────────────────────────────────────────────────────────────────────────
# Visualización de Speedup y predicción de Amdahl para el sistema N-Body.
#
# ENTRADA:  scaling_parallel.dat  (parallel-for) o scaling_tasks.dat
#   Columnas: Threads AvgTime StdDevTime Speedup SpeedupError
#             Efficiency EfficiencyError SerialFraction ParallelFraction
#   Primera fila: encabezado → se salta con 'every ::1'
#
# SALIDA:   speedup_parallel.png
#   Panel superior: Sp(p) medido con barras de error ± σ_Sp
#                   + curva ideal Sp = p
#                   + predicción de Amdahl usando fracción serial promedio
#   Panel inferior: f_s y f_p estimadas por punto mediante Amdahl
#
# USO:
#   gnuplot scripts/plot_speedup.gnu    ← directo
#   make plot                           ← vía Makefile (recomendado)
# ─────────────────────────────────────────────────────────────────────────────

DAT = 'scaling_parallel.dat'

# ── Detectar pmax (número de filas de datos, excluyendo header) ──────────────
# Usamos system() para contar líneas; restamos 1 por el encabezado.
pmax = int(system("awk 'NR>1{c++} END{print c}' ".DAT))
if (pmax == 0) {
    print "ERROR: ".DAT." no encontrado o vacío. Ejecute primero: make benchmark"
    exit status 1
}

# ── Leer fracción serial promedio de las filas p>=2 (p=1 es trivialmente f=1) 
f_sum  = 0.0
f_count = 0
do for [row=2:pmax] {
    # col 8 = SerialFraction; col 1 = Threads
    f_val = real(system("awk 'NR==".( row+1 )." {print $8}' ".DAT))
    # Solo incluir valores físicamente razonables (0 ≤ f ≤ 1)
    if (f_val >= 0.0 && f_val <= 1.0) {
        f_sum   = f_sum + f_val
        f_count = f_count + 1
    }
}
f_serial = (f_count > 0 ? f_sum / f_count : 0.1)
print sprintf("Fracción serial promedio (Amdahl): %.4f", f_serial)

# ── Función de Amdahl ─────────────────────────────────────────────────────────
amdahl(p) = 1.0 / (f_serial + (1.0 - f_serial) / p)

# ── Terminal de salida ────────────────────────────────────────────────────────
set terminal pngcairo size 1000,900 enhanced font 'Helvetica,10' background '#FFFFFF'
set output 'speedup_parallel.png'

set datafile separator whitespace

# ── Estética global ───────────────────────────────────────────────────────────
set grid linestyle 1 lc rgb '#CCCCCC' lw 0.5 dt 2
set lmargin 12
set rmargin 10
set style line 1 lc rgb '#E07B54' lw 2 pt 7 ps 1.2   # medido
set style line 2 lc rgb '#27AE60' lw 1.5 dt 2         # ideal (Sp=p)
set style line 3 lc rgb '#4A90D9' lw 1.5 dt 4         # Amdahl
set style line 4 lc rgb '#9B59B6' lw 1.2 pt 9 ps 1.0  # fracción serial

# ── Multiplot: 2 paneles ──────────────────────────────────────────────────────
set multiplot layout 2,1 \
    title sprintf("Análisis de Escalabilidad N-Body  {/*0.85 (f_{serial} ≈ %.3f)}", f_serial) \
    font ",13"

# ═══════════════════════════════════════════════════════════════════════════════
# PANEL 1: Speedup vs. número de hilos
# ═══════════════════════════════════════════════════════════════════════════════
set title "Speedup S_p = T_1 / T_p" font ",11" offset 0,-0.3
set ylabel "Speedup S_p" font ",10"
unset xlabel
set bmargin 0
set tmargin 2
set xrange [0.5 : pmax + 0.5]
set yrange [0 : *]
set xtics 1

set key top left font ",9" spacing 1.3 box lc rgb '#AAAAAA'

# Línea ideal (Sp = p) — continua sobre [1, pmax]
set samples 200
ideal(p) = p

plot DAT every ::1 using 1:4:5 \
         with yerrorbars ls 1 title 'Medido (±σ)', \
     DAT every ::1 using 1:4 \
         with linespoints ls 1 notitle, \
     [1:pmax] ideal(x) \
         with lines ls 2 title 'Ideal (S_p = p)', \
     [1:pmax] amdahl(x) \
         with lines ls 3 title sprintf('Amdahl (f=%.3f)', f_serial)

# ═══════════════════════════════════════════════════════════════════════════════
# PANEL 2: Fracción serial estimada por punto
# ═══════════════════════════════════════════════════════════════════════════════
set title "Fracciones serial y paralela  {/*0.85 Amdahl: f_s = (1/S_p − 1/p) / (1 − 1/p), f_p = 1 − f_s}" \
    font ",11" offset 0,-0.3
set ylabel "Fracción" font ",10"
set xlabel "Número de hilos p" font ",10"
set bmargin 3
set tmargin 0
set yrange [0 : 1]
set key top right font ",9"

# Líneas horizontales: promedios
set arrow 1 from 0.5, f_serial to pmax+0.5, f_serial \
    nohead lc rgb '#4A90D9' lw 1.5 dt 3
set label 1 sprintf("f̄_s = %.3f", f_serial) \
    at pmax+0.55, f_serial font ",8" tc rgb '#4A90D9'

set style line 5 lc rgb '#E07B54' lw 1.2 pt 5 ps 1.0  # fracción paralela

plot DAT every ::1 using 1:8 \
     with linespoints ls 4 title 'f_s serial', \
     DAT every ::1 using 1:9 \
     with linespoints ls 5 title 'f_p paralela'

unset arrow 1
unset label 1
unset multiplot
print "speedup_parallel.png generado correctamente."
