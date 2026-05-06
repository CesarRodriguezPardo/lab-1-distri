# set terminal pngcairo enhanced font "Arial,12" size 900,600
set terminal pngcairo enhanced font "Arial,12" size 900,600
set output "chunk_plot.png"

set title "Tiempo de cómputo vs. Chunk Size por Schedule (OpenMP)" font "Arial,14"
set xlabel "Chunk Size" font "Arial,12"
set ylabel "Tiempo promedio [s]" font "Arial,12"

# Escala logarítmica en X para ver los chunks 1, 4, 16, 64, 256
set logscale x 2
set xtics (1, 4, 16, 64, 256)
set format x "%g"

set grid
set key top right

# Colores y estilos por schedule
# 1=static (naranja), 2=dynamic (azul), 3=guided (verde)
set style line 1 lc rgb "#E67E22" lw 2 pt 7 ps 1.2   # static  → naranja
set style line 2 lc rgb "#2980B9" lw 2 pt 5 ps 1.2   # dynamic → azul
set style line 3 lc rgb "#27AE60" lw 2 pt 9 ps 1.2   # guided  → verde

# benchmark_results.dat tiene columnas:
#   Schedule  ChunkSize  AvgTime  StdDevTime
# donde Schedule es la palabra "static", "dynamic" o "guided"

# Extraer cada schedule y graficar con barras de error
plot "benchmark_results.dat" \
        using ($1 eq "static"  ? $2 : 1/0):3:4 with yerrorbars ls 1 title "static",  \
     "" using ($1 eq "static"  ? $2 : 1/0):3        with linespoints ls 1 notitle,    \
     "" using ($1 eq "dynamic" ? $2 : 1/0):3:4 with yerrorbars ls 2 title "dynamic", \
     "" using ($1 eq "dynamic" ? $2 : 1/0):3        with linespoints ls 2 notitle,    \
     "" using ($1 eq "guided"  ? $2 : 1/0):3:4 with yerrorbars ls 3 title "guided",  \
     "" using ($1 eq "guided"  ? $2 : 1/0):3        with linespoints ls 3 notitle
