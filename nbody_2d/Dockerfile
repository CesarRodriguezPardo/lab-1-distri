# Base image for the GitHub Actions workflow (and local parity): Debian + C++ build
# tools + Python visualization stack.
# The workflow mounts the repo at /workspace and runs make there;
# this file does not copy or build the project itself.
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

# ── Capa 1: herramientas de compilación C++ ───────────────────────────────────
# Se instalan en una capa separada para aprovechar el cache de Docker:
# si solo cambian las dependencias Python, esta capa no se reconstruye.
RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    ca-certificates \
    g++ \
    make \
  && rm -rf /var/lib/apt/lists/*

# ── Capa 2: gnuplot para generación de gráficos (make plot) ──────────────────
# gnuplot es un paquete del sistema sin dependencias de pip/venv.
# Instala sin fricción en cualquier entorno Debian/Ubuntu.
# Equivalente local en macOS: brew install gnuplot
RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    gnuplot \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["bash"]