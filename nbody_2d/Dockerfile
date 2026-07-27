# Base image for the GitHub Actions workflow (and local parity): CUDA devel
# image + C++ build tools.
# La suite `make test` corre en CPU; el soporte GPU requiere host NVIDIA.
# The workflow mounts the repo at /workspace and runs make there;
# this file does not copy or build the project itself.
FROM nvidia/cuda:12.4.1-devel-ubuntu22.04

ENV DEBIAN_FRONTEND=noninteractive

# ── Capa 1: herramientas de compilación C++ ─────────────────────────────────
# Se instalan en una capa separada para aprovechar el cache de Docker:
# si solo cambian las dependencias Python, esta capa no se reconstruye.
# libgomp1: runtime de OpenMP requerido explícitamente para gomp scheduling.
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    g++ \
    make \
    libgomp1 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Compila y ejecuta los tests automáticamente al arrancar el contenedor.
# Uso: docker run --rm -v $(pwd):/workspace <imagen>
CMD ["bash"]