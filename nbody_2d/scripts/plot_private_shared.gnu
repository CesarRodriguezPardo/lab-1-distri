# plot_private_shared.gnu
# Genera un gráfico de barras comparando el rendimiento de
# private vs shared en calculateEnergy.
#
# Columnas de private_vs_shared.dat:
#   1=Mode  2=Threads  3=AvgTime  4=StdDevTime  5=SpeedupVsSerial

set terminal pngcairo enhanced font "DejaVu Sans,12" size 900,600
set output "private_vs_shared.png"

# ── Estilo general ──────────────────────────────────────────
set title "Benchmark: private vs shared en calculateEnergy" font ",14" offset 0,0.5
set style data histograms
set style histogram errorbars lw 1.5 gap 2
set style fill solid 0.75 border -1

set boxwidth 0.6
set yrange [0:]
set ylabel "Tiempo promedio por llamada (s)" font ",12"
set xlabel "Modo de clausula OpenMP" font ",12"

set grid ytics lt 0 lw 1 lc rgb "#cccccc"
set border 3
set tics nomirror

set key top right font ",11"

# ── Paleta de colores ────────────────────────────────────────
set linetype 1 lc rgb "#2563eb"  # azul — private
set linetype 2 lc rgb "#dc2626"  # rojo — shared

# ── Etiquetas de xtics desde la columna Mode ─────────────────
# Gnuplot trata la primera columna como string para histogramas:
set xtics ("private" 0, "shared" 1)

# ── Plot ─────────────────────────────────────────────────────
plot "private_vs_shared.dat" \
        using 3:4:xtic(1) \
        title "Tiempo ± StdDev" \
        with histograms lc rgb "#2563eb" lw 1.5

# ── Segundo gráfico: Speedup vs Serial ───────────────────────
set output "private_vs_shared_speedup.png"
set title "Speedup (private vs shared) relativo al serial" font ",14"
set yrange [0:]
set ylabel "Speedup vs 1 hilo (mayor = mejor)" font ",12"
set style data histograms
set style histogram clustered gap 1

plot "private_vs_shared.dat" \
        using 5:xtic(1) \
        title "Speedup vs serial" \
        with histograms lc rgb "#16a34a" lw 1.5
