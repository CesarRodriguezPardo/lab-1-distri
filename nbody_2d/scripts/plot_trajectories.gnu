# plot_trajectories.gnu
# ─────────────────────────────────────────────────────────────────────────────
# Visualización de trayectorias del sistema N-Body.
#
# ENTRADA:  trajectories_<modo>.dat  (el primero que exista se usa)
#   Modos reconocidos: serial, parallel_critical, parallel_nowait,
#                      tasks_critical, tasks_nowait
#   Formato: id \t step \t X \t Y \t VX \t VY
#   Separador: TAB
#   Primera fila: encabezado.
#
# SALIDAS:
#   trajectories_plot.png  — trayectorias de hasta los primeros 8 cuerpos
#   position_vs_time.png   — X(t) e Y(t) del cuerpo 0
#
# ── ESTRATEGIA DE FILTRADO ────────────────────────────────────────────────────
# El archivo tiene N cuerpos × S pasos = N×S filas.
# Con N cuerpos, el cuerpo B aparece en las filas B, B+N, B+2N,...
# Usamos 'every N::B' para seleccionar solo las filas de ese cuerpo.
# N_BODIES se detecta automáticamente contando IDs únicos en el archivo.
# ─────────────────────────────────────────────────────────────────────────────

set datafile separator "\t"

# ── Detección dinámica del archivo de trayectorias ────────────────────────────
labels_traj = "serial parallel_critical parallel_nowait tasks_critical tasks_nowait"
file_exists(f) = int(system("[ -f '".f."' ] && echo 1 || echo 0"))

traj_file = ""
do for [i=1:words(labels_traj)] {
    lbl = word(labels_traj, i)
    f = "trajectories_".lbl.".dat"
    if (file_exists(f) && traj_file eq "") {
        traj_file = f
        print "Usando archivo de trayectorias: ".f
    }
}

if (traj_file eq "") {
    print "ERROR: No se encontró ningún archivo trajectories_*.dat."
    print "Ejecute primero: make analysis"
    exit status 1
}

# ── Detección automática de N_BODIES ─────────────────────────────────────────
N_BODIES = int(system("awk 'NR>1{ids[$1]=1} END{print length(ids)}' ".traj_file))
if (N_BODIES == 0) { N_BODIES = 100 }
print "N_BODIES detectado: ".N_BODIES

# Número de cuerpos a graficar (máximo 8)
N_PLOT = (N_BODIES < 8 ? N_BODIES : 8)

# ─────────────────────────────────────────────────────────────────────────────
# FIGURA 1: Trayectorias en el plano (X, Y) — fondo oscuro
# ─────────────────────────────────────────────────────────────────────────────
set terminal pngcairo size 1000,900 enhanced font 'Helvetica,10' \
    background '#0F0F1A'
set output 'trajectories_plot.png'

set title "Trayectorias N-Body  {/*0.85 (primeros ".N_PLOT." cuerpos)}" \
    font ",13" textcolor rgb '#FFFFFF'
set xlabel "X [u.a.]" font ",11" textcolor rgb '#CCCCCC'
set ylabel "Y [u.a.]" font ",11" textcolor rgb '#CCCCCC'
set grid lc rgb '#333333' lw 0.4 dt 2
set border lc rgb '#555555'
set tics textcolor rgb '#CCCCCC'
set key outside top right textcolor rgb '#DDDDDD' \
    box lc rgb '#444444' spacing 1.3 font ",9"

# Paleta de 8 colores distinguibles sobre fondo oscuro
colors_traj = "#FF6B6B #4ECDC4 #FFE66D #A8E6CF #FF8B94 #88D8B0 #FCBF49 #6C63FF"

# Construir comando de plot dinámicamente
cmd_traj = ""
do for [b=0:N_PLOT-1] {
    clr = word(colors_traj, b+1)
    sep = (cmd_traj eq "" ? "" : ", ")
    cmd_traj = cmd_traj.sep \
        ."'".traj_file."' skip 1 every ".N_BODIES."::".b \
        ." using 3:4 with lines lw 0.9 lc rgb '".clr."' title 'Cuerpo ".b."'"
}
eval("plot ".cmd_traj)

print "trajectories_plot.png generado correctamente."

# ─────────────────────────────────────────────────────────────────────────────
# FIGURA 2: X(t) e Y(t) del cuerpo 0 — fondo blanco
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
plot traj_file skip 1 every N_BODIES::0 using 2:3 \
    with lines lw 1.2 lc rgb '#4ECDC4' notitle

# Panel Y(t)
set title "Coordenada Y(t)  — cuerpo 0" font ",10" offset 0,-0.3
set ylabel "Y [u.a.]" font ",9"
set xlabel "Paso de simulación" font ",10"
set bmargin 3
set tmargin 0
plot traj_file skip 1 every N_BODIES::0 using 2:4 \
    with lines lw 1.2 lc rgb '#FF6B9D' notitle

unset multiplot
print "position_vs_time.png generado correctamente."
