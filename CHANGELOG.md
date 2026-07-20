# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [2.0.0-lab2] - 2026-07-24

### Added

- Kernels CUDA para `computeAccelerations`: variante básica (un hilo por cuerpo `i`, bucle interno sobre `j`) y variante con memoria compartida (tiles de posiciones y masas en `__shared__` con `__syncthreads()`), físicamente equivalentes dentro de la tolerancia acordada.
- Gestión de memoria host/device con `CudaBuffer` (RAII para `cudaMalloc`/`cudaFree`) y layout SoA en device (`d_mass`, `d_x`, `d_y`, ...) para favorecer coalescing.
- Integración Euler explícita en host tras sincronizar el device con `cudaDeviceSynchronize()`, siguiendo el orden fijo del paso temporal del enunciado.
- Tests de equivalencia CPU (referencia serial del Lab 1) vs. GPU con tolerancia documentada (`rtol = 1e-4`, `atol = 1e-8` para aceleraciones).
- Cálculo de energías cinética `K` y potencial `U` en GPU con reducción paralela en shared memory y una variante con `atomicAdd`.
- Manejo de errores CUDA con macro `CUDA_CHECK` tras llamadas a la API y `cudaGetLastError` tras lanzamientos de kernels.
- Tres agentes de IA ejecutados en CI (`.github/agents/`): documentador, revisor de bugs y revisor de merge requests, con criterio explícito de cambio mecánico vs. intervención humana.
- Protección de la rama `main`: sin push directo, merge solo vía PR, una revisión humana obligatoria y CI verde requerido.
- Flujo de trabajo con ramas `feature/*` y `fix/*`, issues etiquetados y PRs vinculados (`Closes #N`).
- CI extendido del Lab 1: compilación y `make test` obligatorios en cada PR.
- Documentación del flujo Git en `.github/GIT_FLOW.md`.

### Changed

- `NBodySystem` y `NBodySimulator` extendidos con sobrecargas para variantes GPU (`computeAccelerationsGpu`, `stepEulerGpu`, `calculateEnergyGpu`) sin duplicar la lógica física.
- Mediciones de tiempo con `std::chrono::steady_clock` en host alrededor de `cudaDeviceSynchronize()` (sin `cudaEvent`), separando kernel-only de end-to-end.
- El repositorio opera como un proyecto de software real: `main` protegida, CHANGELOG y releases etiquetadas.

### Fixed

- Tolerancias de comparación en coma flotante justificadas para la regresión CPU/GPU (la aritmética IEEE 754 no es asociativa).
- Límite de 5 issues automáticos por agente por semana para evitar ruido en el tablero.
