# plot_system.gnu

set terminal pngcairo size 800,800

# -------------------------------------------------------------------
# Detectar automáticamente qué archivo existe
# -------------------------------------------------------------------

labels = "binary disk random"

file_exists(f) = int(system("[ -f '".f."' ] && echo 1 || echo 0"))

system_file = ""
system_name = ""

do for [i=1:words(labels)] {

    lbl = word(labels, i)
    f = lbl."_system.dat"

    if (file_exists(f) && system_file eq "") {
        system_file = f
        system_name = lbl
    }
}

# -------------------------------------------------------------------
# Verificar si se encontró un archivo
# -------------------------------------------------------------------

if (system_file eq "") {
    print "ERROR: No se encontró ningún archivo *_system.dat"
    exit status 1
}

print "Usando archivo: ".system_file

# -------------------------------------------------------------------
# Configuración del gráfico
# -------------------------------------------------------------------

set output system_name."_system.png"

set title "Sistema ".system_name

set xlabel "X"
set ylabel "Y"

set size square

set xrange [0:100]
set yrange [0:100]

set grid

# -------------------------------------------------------------------
# Graficar
# -------------------------------------------------------------------

plot system_file using 1:2 with points pt 7 ps 0.7 title "Particulas"