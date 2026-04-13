# Se define una imagen multi-stage para el proyecto

# Stage Builder: se encarga de compilar el proyecto

# se utiliza debian:bookworm-slim como base para asegurar compatibilidad de librerías
FROM debian:bookworm-slim AS builder 

# Instalar dependencias para compilar
RUN apt-get update && apt-get install -y g++ make

# se define el directorio app dentro del contenedor y se copia
# todo el contenido del proyecto
WORKDIR /app
COPY . .

# utilizamos make para compilar
RUN make all

#############################################

# Stage Runtime: se encarga de ejecutar el proyecto
FROM debian:bookworm-slim AS runtime

# se define el directorio app y se copia el binario resultante de ejecutar
# make all
WORKDIR /app
COPY --from=builder /app/nbody .

CMD ["./nbody"]

# en el main se debe poder ejecutar la simulación y se deben poder parametrizar las constantes