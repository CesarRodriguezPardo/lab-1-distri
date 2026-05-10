# plot_energy.gnu
# ─────────────────────────────────────────────────────────────────────────────
# Visualización de la energía del sistema N-Body a lo largo del tiempo.
#
# ENTRADA:  energy_<modo>.dat  (uno o varios según el modo elegido)
#   Modos reconocidos: serial, parallel_critical, parallel_nowait,
#                      tasks_critical, tasks_nowait
#   Formato: K_Cinetica  U_Potencial  E_Total  (separado por whitespace)
#   Primera fila: encabezado → se salta con 'every ::1'
#
# SALIDA:   energy_plot.png
#   Tres paneles apilados: K(t), U(t), E = K + U (t)
#   Si hay varios archivos (modo 5 = todos) se superponen con colores distintos.
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
set terminal pngcairo size 1100,900 enhanced font 'Helvetica,10' background '#FFFFFF'
set output 'energy_plot.png'

set datafile separator whitespace

# ── Estética global ───────────────────────────────────────────────────────────
set grid linestyle 1 lc rgb '#CCCCCC' lw 0.5 dt 2
set lmargin 12
set rmargin 4

# ── Detección dinámica de archivos ────────────────────────────────────────────
# Modos conocidos (en orden) y sus colores asociados
labels = "serial parallel_critical parallel_nowait tasks_critical tasks_nowait parallel_for"
colors = "#E07B54 #4A90D9 #27AE60 #9B59B6 #F39C12 #34495E"

file_exists(f) = int(system("[ -f '".f."' ] && echo 1 || echo 0"))

# Construir comandos de plot para cada panel
cmd_k = ""
cmd_u = ""
cmd_e = ""
do for [i=1:words(labels)] {
    lbl = word(labels, i)
    clr = word(colors, i)
    f = "energy_".lbl.".dat"
    if (file_exists(f)) {
        sep = (cmd_k eq "" ? "" : ", ")
        cmd_k = cmd_k.sep."'".f."' every ::1 using 0:1 with lines lc rgb '".clr."' lw 1.3 title '".lbl."'"
        cmd_u = cmd_u.sep."'".f."' every ::1 using 0:2 with lines lc rgb '".clr."' lw 1.3 title '".lbl."'"
        cmd_e = cmd_e.sep."'".f."' every ::1 using 0:3 with lines lc rgb '".clr."' lw 1.3 title '".lbl."'"
    }
}

if (cmd_k eq "") {
    print "ERROR: No se encontró ningún archivo energy_*.dat."
    print "Ejecute primero: make analysis"
    exit status 1
}

# ── Multiplot: 3 paneles apilados ─────────────────────────────────────────────
set multiplot layout 3,1 \
    title "Energía del sistema N-Body — Integrador de Euler explícito\n" \
    font ",13"

# ── PANEL 1: K(t) — Energía cinética ─────────────────────────────────────────
set title "K — Energía cinética" font ",10" offset 0,-0.3
set ylabel "K [u.a.]" font ",9"
unset xlabel
set bmargin 0
set tmargin 2
set key top right font ",8" spacing 1.2
eval("plot ".cmd_k)

# ── PANEL 2: U(t) — Energía potencial ────────────────────────────────────────
set title "U — Energía potencial" font ",10" offset 0,-0.3
set ylabel "U [u.a.]" font ",9"
unset xlabel
set bmargin 0
set tmargin 0
unset key
eval("plot ".cmd_u)

# ── PANEL 3: E(t) = K + U — Energía total ────────────────────────────────────
set title "E = K + U — Energía total  {/*0.85 (deriva esperada con Euler)}" \
    font ",10" offset 0,-0.3
set ylabel "E [u.a.]" font ",9"
set xlabel "Paso de simulación" font ",10"
set bmargin 3
set tmargin 0
eval("plot ".cmd_e)

unset multiplot
print "energy_plot.png generado correctamente."
