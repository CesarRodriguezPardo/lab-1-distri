# plot_trajectories.gnu
# ─────────────────────────────────────────────────────────────────────────────
# Visualización de trayectorias del sistema N-Body.
#
# ENTRADA:  trajectories.dat
#   Formato: id \t step \t X \t Y \t VX \t VY
#   Separador: TAB (el archivo usa " \t " espacio-tab-espacio)
#   Primera fila: encabezado.
#
# SALIDAS:
#   trajectories_plot.png  — trayectorias de los primeros 8 cuerpos
#   position_vs_time.png   — X(t) e Y(t) del cuerpo 0
#
# ── ESTRATEGIA DE FILTRADO ────────────────────────────────────────────────────
# El archivo tiene N cuerpos × S pasos = N×S filas.
# Con 100 cuerpos, el cuerpo 0 aparece en las filas 0, 100, 200, 300...
# Usamos 'every N' para saltar de 100 en 100, empezando en la fila del cuerpo.
#   every <paso>::<inicio>
#   - <paso>=N_BODIES: avanza N_BODIES filas entre puntos (siguiente aparición del mismo cuerpo)
#   - <inicio>=ID:     primera fila del cuerpo ID (0-indexed tras skip 1)
#
# Esto evita el problema de NaN entre puntos que rompe las líneas.
# ─────────────────────────────────────────────────────────────────────────────

set datafile separator "\t"

N_BODIES = 100   # número de cuerpos en la simulación (debe coincidir con main.cpp)

# ─────────────────────────────────────────────────────────────────────────────
# FIGURA 1: Trayectorias en el plano (X, Y) — fondo oscuro
# ─────────────────────────────────────────────────────────────────────────────
set terminal pngcairo size 1000,900 enhanced font 'Helvetica,10' \
    background '#0F0F1A'
set output 'trajectories_plot.png'

set title "Trayectorias N-Body  {/*0.85 (primeros 8 cuerpos)}" \
    font ",13" textcolor rgb '#FFFFFF'
set xlabel "X [u.a.]" font ",11" textcolor rgb '#CCCCCC'
set ylabel "Y [u.a.]" font ",11" textcolor rgb '#CCCCCC'
set grid lc rgb '#333333' lw 0.4 dt 2
set border lc rgb '#555555'
set tics textcolor rgb '#CCCCCC'
set key outside top right textcolor rgb '#DDDDDD' \
    box lc rgb '#444444' spacing 1.3 font ",9"

# Paleta de 8 colores distinguibles sobre fondo oscuro
c0 = '#FF6B6B'
c1 = '#4ECDC4'
c2 = '#FFE66D'
c3 = '#A8E6CF'
c4 = '#FF8B94'
c5 = '#88D8B0'
c6 = '#FCBF49'
c7 = '#6C63FF'

# 'every N_BODIES::ID' selecciona filas: ID, ID+N, ID+2N,...
# (fila 0-indexed contando desde el primer dato, es decir tras skip 1)
# col 3 = X, col 4 = Y
plot \
  'trajectories.dat' skip 1 every N_BODIES::0 using 3:4 \
      with lines lw 0.9 lc rgb c0 title 'Cuerpo 0', \
  'trajectories.dat' skip 1 every N_BODIES::1 using 3:4 \
      with lines lw 0.9 lc rgb c1 title 'Cuerpo 1', \
  'trajectories.dat' skip 1 every N_BODIES::2 using 3:4 \
      with lines lw 0.9 lc rgb c2 title 'Cuerpo 2', \
  'trajectories.dat' skip 1 every N_BODIES::3 using 3:4 \
      with lines lw 0.9 lc rgb c3 title 'Cuerpo 3', \
  'trajectories.dat' skip 1 every N_BODIES::4 using 3:4 \
      with lines lw 0.9 lc rgb c4 title 'Cuerpo 4', \
  'trajectories.dat' skip 1 every N_BODIES::5 using 3:4 \
      with lines lw 0.9 lc rgb c5 title 'Cuerpo 5', \
  'trajectories.dat' skip 1 every N_BODIES::6 using 3:4 \
      with lines lw 0.9 lc rgb c6 title 'Cuerpo 6', \
  'trajectories.dat' skip 1 every N_BODIES::7 using 3:4 \
      with lines lw 0.9 lc rgb c7 title 'Cuerpo 7'

print "trajectories_plot.png generado correctamente."

# ─────────────────────────────────────────────────────────────────────────────
# FIGURA 2: X(t) e Y(t) del cuerpo 0 — fondo blanco
# ─────────────────────────────────────────────────────────────────────────────
# col 2 = step, col 3 = X, col 4 = Y
# 'every N_BODIES::0' selecciona solo filas del cuerpo 0
# ─────────────────────────────────────────────────────────────────────────────
set terminal pngcairo size 1000,600 enhanced font 'Helvetica,10' \
    background '#FFFFFF'
set output 'position_vs_time.png'

set title "Posición del cuerpo 0 en función del paso de simulación" \
    font ",13" textcolor rgb '#000000'
set grid lc rgb '#CCCCCC' lw 0.5 dt 2
set border lc rgb '#000000'
set tics textcolor rgb '#000000'
unset key
set lmargin 12
set rmargin 4

set multiplot layout 2,1

# Panel X(t)
set title "Coordenada X(t)  — cuerpo 0" font ",10" offset 0,-0.3
set ylabel "X [u.a.]" font ",9"
unset xlabel
set bmargin 0
set tmargin 2
plot 'trajectories.dat' skip 1 every N_BODIES::0 using 2:3 \
    with lines lw 1.2 lc rgb '#4ECDC4' notitle

# Panel Y(t)
set title "Coordenada Y(t)  — cuerpo 0" font ",10" offset 0,-0.3
set ylabel "Y [u.a.]" font ",9"
set xlabel "Paso de simulación" font ",10"
set bmargin 3
set tmargin 0
plot 'trajectories.dat' skip 1 every N_BODIES::0 using 2:4 \
    with lines lw 1.2 lc rgb '#FF6B9D' notitle

unset multiplot
print "position_vs_time.png generado correctamente."
