# Laboratorio 2: Simulador Gravitatorio N-Cuerpos 2D — C++/CUDA

**Versión:** `v2.0.0-lab2` (tag de entrega, creado desde `main` mediante el workflow manual `release_tag.yml`)
**Curso:** Sistemas Distribuidos — USACH 2026
**Repositorio:** [https://github.com/CesarRodriguezPardo/lab-1-distri](https://github.com/CesarRodriguezPardo/lab-1-distri)

Este repositorio contiene una simulación computacional del problema gravitatorio de los N-Cuerpos en 2D. Sobre la base CPU del Laboratorio 1 (serial + OpenMP, validada con Catch2), el Laboratorio 2 incorpora **kernels CUDA en GPU** (dos variantes), gestión de memoria host/device con layout **SoA**, validación numérica CPU vs GPU con tolerancias explícitas, **Integración Continua en contenedor Docker**, **tres agentes de IA operando sobre el flujo Git** y una **matriz de benchmarks reproducible en el clúster DIINF**.

---

## 1. Identificación del Proyecto y Tabla de Integrantes (Sección 3)

| Nombre                | Rol                                | Rutas de código/scripts a su cargo                                                                                                                                                                                | Responsabilidades concretas                                                                                                                                                                                                                                                                                                                                                                                |
| :-------------------- | :--------------------------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Sofía Gacitúa**     | Rol 1: Kernels CUDA                | `nbody_2d/kernels/accelerations.cu`, `nbody_2d/kernels/accelerations.cuh`, `nbody_2d/kernels/metrics.cu`, `nbody_2d/kernels/metrics.cuh`                                                                          | Kernel básico de aceleraciones (un hilo por cuerpo `i`), kernel con memoria compartida (`extern __shared__`, tiling, doble `__syncthreads()` por tile), protección de bordes (`if (i >= N) return;`), kernels de energía K/U (variantes `atomicAdd` y reducción en shared), parche `atomicAdd(double)` vía CAS para `__CUDA_ARCH__ < 600` , selección de variante de kernel (`variant` 0/1) y `blockSize`. |
| **Martín Salinas**    | Rol 2: Host/Device y Memoria       | RAII `nbody_2d/CudaBuffer.h`, `nbody_2d/NBodySimulator.cpp`, Sincronización correcta en `nbody_2d/`.                                                                                                              | Wrapper RAII de memoria device (7 `cudaMalloc`, `cudaFree` en destructor, copia prohibida, move semantics), conversión AoS→SoA en host, transferencias H2D/D2H (`updateDeviceKinematics`, `retrieveAccelerations`), macro `CUDA_CHECK`.                                                                                                                                                                    |
| **Nicolás García**    | Rol 3: Integración y Validación    | `nbody_2d/Integrator.cpp` (`integrateEulerGpu`), `nbody_2d/NBodySimulator.cpp` (simulación CUDA, `calculateEnergyGpu`), `nbody_2d/Benchmark.cpp` (`compareCpuGpu`, matriz GPU), `nbody_2d/main.cpp` (modos 2 y 3) | Integración Euler en host tras `cudaDeviceSynchronize()` (D2H de aceleraciones → kick/drift en CPU → H2D de kinematics), validación numérica CPU vs GPU con `rtol=1e-4` / `atol=1e-8`, implementación de la matriz obligatoria de benchmarks y exportación a `benchmark_results.dat`.                                                                                                                      |
| **César Rodríguez**   | Rol 4: Git, Releases y Agentes     | `.github/agents/*.py`, `.github/workflows/agent_documenter.yml`, `agent_bug_reviewer.yml`, `agent_mr_reviewer.yml`, `.github/workflows/release_tag.yml`, `.github/GIT_FLOW.md`, `CHANGELOG.md`                    | Implementación de los tres agentes de IA (PRs #24, #44, #48), reglas de protección de `main` y flujo `feature/*` + `fix/*` con vinculación `Closes #N`, mantención del CHANGELOG (Keep a Changelog), automatización del tag/release `v2.0.0-lab2`.                                                                                                                                                         |
| **Sebastián Cassone** | Rol 5: Calidad, CI y Visualización | `.github/workflows/ci.yml`, `build_base_container.yml`, `nbody_2d/Dockerfile`, `nbody_2d/Dockerfile.cuda`, `nbody_2d/Makefile`, `nbody_2d/scripts/generate_plots.py`, `nbody_2d/tests/`                           | Pipeline CI en contenedor (compilación `nvcc` + `make test`), Dockerfiles, ejecución manual de benchmarks en clúster DIINF vía Slurm, script de figuras `performance_plots.png`, suite de tests Catch2 y targets del Makefile.                                                                                                                                                                             |

---

## 2. Requisitos de Entorno y Compilación (Sección 13)

### 2.1 Requisitos de software

| Software                                       | Requisito                               | Uso en el proyecto                                     |
| :--------------------------------------------- | :-------------------------------------- | :----------------------------------------------------- |
| **GCC / G++**                                  | Con soporte **C++17** (`-std=c++17`)    | Código host C++ (compilado a través de `nvcc`)         |
| **NVCC / CUDA Toolkit**                        | **12.x** (imagen de referencia: 12.4.1) | Compilación de kernels `.cu` y enlazado con `-lcudart` |
| **Drivers NVIDIA**                             | Compatibles con CUDA 12.x               | Ejecución en GPU (clúster DIINF / host con GPU)        |
| **OpenMP** (`libgomp1`)                        | Incluido con GCC                        | Rutas CPU paralelas heredadas del Lab 1                |
| **GNU Make**                                   | Cualquier versión reciente              | Orquestación de la compilación                         |
| **Python 3** + `numpy`, `pandas`, `matplotlib` | Para visualización Lab 2                | `scripts/generate_plots.py`                            |
| **Gnuplot**                                    | Opcional (legado Lab 1)                 | Scripts `scripts/*.gnu`                                |

### 2.2 Compilación mediante Makefile

Todo el proyecto se compila con `nvcc` (también los `.cpp`, para enlazar el runtime CUDA). Flags aplicados, verificados en `nbody_2d/Makefile`:

```makefile
NVCCFLAGS = -O3 -std=c++17 -I. -Ikernels -Xcompiler "-Wall,-Wextra,-fopenmp"
LDFLAGS   = -lcudart -Xcompiler -fopenmp
```

Es decir: **`-O3`** (optimización agresiva), **`-Wall -Wextra`** (warnings completos, propagados al compilador host vía `-Xcompiler`), **C++17** y OpenMP. No se fijan flags `-arch`/`-gencode`: se usa la arquitectura por defecto de `nvcc`.

Targets disponibles (ejecutar dentro de `nbody_2d/`):

| Target                   | Acción                                                                                                                                                                                                    |
| :----------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `make` / `make all`      | Compila el binario interactivo principal **`nbody`** (`main.cpp` + fuentes + kernels).                                                                                                                    |
| `make test`              | Compila **`test_runner`** (suite Catch2 v3: `test_Particle`, `test_NBodySystem`, `test_Integrator`, `test_MetricsCalculator`) **y la ejecuta**; exit code ≠ 0 si algún test falla. Corre en CPU, sin GPU. |
| `make benchmark-bin`     | Compila el binario **`benchmark`** (benchmarks CPU) sin ejecutarlo.                                                                                                                                       |
| `make benchmark`         | Compila y ejecuta `./benchmark` → genera los `.dat` de escalamiento CPU (`scaling_parallel.dat`, `chunk_schedule.dat`, etc.). Este target también se ejecuta en CI.                                       |
| `make analysis`          | Ejecuta `./nbody` → genera `energy_<modo>.dat` y `trajectories_<modo>.dat`.                                                                                                                               |
| `make plot`              | Genera los PNG legados del Lab 1 con gnuplot.                                                                                                                                                             |
| `make performance-plots` | Ejecuta `python3 scripts/generate_plots.py` → **`performance_plots.png`** (figuras Lab 2).                                                                                                                |
| `make clean`             | Elimina binarios, `*.dat` y `*.png`.                                                                                                                                                                      |

```bash
cd nbody_2d
make clean && make        # compila ./nbody
make test                 # compila y corre la suite Catch2
```

### 2.3 Uso de Docker (Sección 13.2)

La imagen de contenedor definida en [`nbody_2d/Dockerfile`](nbody_2d/Dockerfile) usa la base **`nvidia/cuda:12.4.1-devel-ubuntu22.04`** e instala `g++`, `make` y `libgomp1`. Comandos exactos (desde `nbody_2d/`):

```bash
docker build -t nbody_cuda -f Dockerfile .
docker run --rm nbody_cuda make test
```

> **Nota operativa importante:** el `Dockerfile` no copia las fuentes a la imagen (`WORKDIR /workspace`, `CMD ["bash"]`); al igual que en `ci.yml`, el código se **monta** en tiempo de ejecución. Por ello el comando de ejecución real es con volumen:
>
> ```bash
> docker run --rm -v "$(pwd):/workspace" -w /workspace nbody_cuda make test
> ```
>
> La suite `make test` corre en CPU y no requiere GPU. Para pasar una GPU al contenedor se necesita además host NVIDIA + driver CUDA 12.x + `nvidia-container-toolkit` (y la bandera `--gpus all`). Existe una imagen separada, [`nbody_2d/Dockerfile.cuda`](nbody_2d/Dockerfile.cuda), utilizada para pruebas locales opcionales con CUDA.

---

## 3. Convenciones CUDA, Layout y Tolerancia Física (Secciones 4.1 y 5.1)

### 3.1 Layout de datos: Structure of Arrays (SoA)

La memoria device se organiza como **Structure of Arrays (SoA)**: en lugar de un arreglo de structs `Particle` (AoS), `CudaBuffer` reserva **7 arreglos `double` independientes** en la GPU (`CudaBuffer.h:24-33`):

```cpp
double *d_mass, *d_x, *d_y, *d_ax, *d_ay, *d_vx, *d_vy;   // un cudaMalloc por arreglo
```

El host mantiene el AoS clásico (`std::vector<Particle> bodies`) y el constructor de `CudaBuffer` lo desempaqueta a arreglos SoA antes de las transferencias H2D. **Justificación:** en el bucle interno del kernel, todos los hilos de un warp leen `d_x[j]`, `d_y[j]`, `d_mass[j]` para el mismo `j` consecutivo; con SoA esos accesos son contiguos entre hilos, produciendo **transacciones de memoria coalesced** y aprovechando el ancho de banda del dispositivo, lo que con AoS (stride de `sizeof(Particle)`) se desperdicia.

### 3.2 Variantes de kernel implementadas (`nbody_2d/kernels/accelerations.cu`)

1. **Variante básica — un hilo por cuerpo `i`** (`computeAccelerationsKernel`, línea 6; seleccionada con `variant=0`):
   cada hilo global `i = blockIdx.x * blockDim.x + threadIdx.x` calcula la aceleración total del cuerpo `i` iterando sobre todos los `j ∈ [0, N)` desde memoria global, con guardia de bordes `if (i >= N) return;`.
2. **Variante con memoria compartida** (`computeAccelerationsKernelShared`, línea 41; `variant=1`):
   el bloque carga colaborativamente tiles de `blockDim.x` cuerpos en `extern __shared__ double shared[]` (particionado en `sharedMass`, `sharedX`, `sharedY` → `3 * blockDim.x * sizeof(double)` bytes dinámicos), con **dos `__syncthreads()` por tile** (tras la carga y tras el cómputo) y manejo explícito del último tile parcial. Reduce lecturas a memoria global de O(N²) a O(N²/blockDim).

La selección se hace en `NBodySystem::computeAccelerationsGpu(int variant, int blockSize)` (`NBodySystem.cpp:192`); `blockSize` por defecto = **256**. Ambas variantes comparten la misma firma SoA: `(d_mass, d_x, d_y, d_ax, d_ay, G, eps, N)`. Adicionalmente existen kernels de energía (`kernels/metrics.cu`) en dos variantes: reducción en memoria compartida (método 0) y `atomicAdd` global (método 1).

### 3.3 Criterio de tolerancia numérica CPU vs GPU

La validación de aceleraciones CPU vs GPU vive en `Benchmark::compareCpuGpu(int n)` (`Benchmark.cpp:344`, invocada desde el modo 2 del binario `nbody`). Valores explícitos (`Benchmark.cpp:375-389`):

```cpp
double rtol = 1e-4;   // tolerancia relativa
double atol = 1e-8;   // tolerancia absoluta (piso para valores cercanos a cero)
// Criterio por componente:  |a_CPU - a_GPU| <= atol + rtol * |a_CPU|
```

**Justificación (IEEE 754):** la aritmética de coma flotante **no es asociativa** — $(a+b)+c \neq a+(b+c)$. CPU y GPU suman las N contribuciones de fuerza en órdenes distintos (y la GPU puede usar FMA con redondeo intermedio distinto), por lo que exigir igualdad binaria (`==`) produciría falsos negativos sistemáticos. Con `double` (≈15-16 dígitos significativos), el error de redondeo acumulado en una suma de N ≈ 10³-10⁴ términos de magnitudes heterogéneas queda muy por debajo de `rtol = 10⁻⁴`; a la vez, esa cota es suficientemente estricta para detectar errores reales (índices mal calculados, lecturas fuera de tile, condiciones de carrera), que producen desviaciones de O(1) o catastróficas. `atol = 10⁻⁸` actúa de piso cuando `|a_CPU|` es casi nulo y la tolerancia relativa sería demasiado exigente. La comparación usa configuración fija y determinista: `G=1.0`, `eps=0.01`, semilla `42`, kernel básico, `blockSize=256`, contra el baseline **CPU serial** de `computeAccelerations()` (Lab 1).

En la suite Catch2 (CPU) se usan márgenes absolutos más estrictos para física analítica: `Approx().margin(1e-12)` (aceleraciones/integrador), `1e-14` (reproducibilidad bit-a-bit serial vs paralelo) y `1e-10` (energías).

**Parámetros físicos del simulador:** sistema de unidades adimensional con `G = 1.0`; `dt = 0.01` por paso (integrador Euler explícito); suavizado `epsilon` configurable (0.1 en simulaciones, 0.01 en benchmarks) para evitar la divergencia de la fuerza a distancias ~0.

---

## 4. Flujo Git, Issues y Estrategia de Ramas (Sección 6)

Documento normativo completo: [`.github/GIT_FLOW.md`](.github/GIT_FLOW.md). Reglas declaradas:

- **Protección de `main`:** prohibido push directo (incluye administradores). El merge a `main` se realiza **únicamente mediante Pull Request** con **≥ 1 aprobación humana** y **CI en verde** (`CI — N-Body 2D`: compilación + `make test`). Los jobs GPU son opcionales y no bloquean; las mediciones de rendimiento solo provienen del clúster DIINF. Las ramas fusionadas se borran automáticamente.
- **Nomenclatura de ramas:** `feature/<nombre>` para funcionalidades y `fix/<nombre>` para correcciones (ejemplos reales: `feature/CUDA`, `fix/CudaBuffer-RAII`).
- **Vinculación obligatoria de issues:** todo MR debe referenciar al menos un issue con `Closes #N` (lo cierra al fusionar) o `Refs #N` (relacionado). Los PRs sin issue asociado pueden penalizarse en la rúbrica de Git; el agente revisor de MRs lo detecta y lo advierte automáticamente.
- **Issues:** mínimo 5 durante el laboratorio, al menos 1 por rol, con título, descripción, etiquetas y asignado (issues oficiales de roles: #20-#23).
- **Commits convencionales:** `<tipo>(<scope>): <descripción>`; tipos `feat/fix/docs/test/ci/chore`, scopes `cuda/memory/testing/ci/agents/git/docs`.
- **Releases:** historial de cambios en [`CHANGELOG.md`](CHANGELOG.md), estructurado según **Keep a Changelog 1.1.0** (entrada `[2.0.0-lab2] - 2026-07-24` con secciones `Added` / `Changed` / `Fixed`). El tag `v2.0.0-lab2` se crea sobre `main` con el workflow manual `release_tag.yml`.

---

## 5. Configuración y Operación de Agentes de IA (Sección 7)

Tres agentes implementados como scripts Python en [`.github/agents/`](.github/agents/) (`documenter.py`, `bug_reviewer.py`, `mr_reviewer.py`, utilidades en `common.py`), ejecutados por workflows dedicados. Usan la API REST de GitHub y, opcionalmente, **Gemini `gemini-2.5-flash`** (sin `GEMINI_API_KEY` operan solo con reglas deterministas). **Guardarraíles comunes:** contenido del repo en solo lectura; únicas salidas seguras = crear issues y comentar PRs; **nunca modifican código fuera de su alcance ni fusionan a `main`**; deduplicación por título y límite de **5 issues automáticos por semana** (`MAX_AUTO_ISSUES_PER_WEEK`).

| Agente                         | Workflow / frecuencia                                                                                                            | Alcance y criterio autogestionado vs humano                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| :----------------------------- | :------------------------------------------------------------------------------------------------------------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Documentador** (Sec. 7.1)    | `agent_documenter.yml` — cron **`0 9 * * 1`** (lunes 09:00 UTC), push a `main` y manual                                          | Revisa `README.md` y `CHANGELOG.md`. Hallazgos **mecánicos** (URL del repo vacía, enlaces `[]()` rotos, entrada `2.0.0-lab2` o secciones `Added/Changed/Fixed` faltantes) → abre **PR de auto-fix** en rama `fix/docs/<slug>` con etiqueta `agent:auto-fix`. Hallazgos de fondo (vía Gemini, prefijo `HUMANO:`) → abre issue _"Requiere intervención humana"_.                                                                                                                     |
| **Revisor de Bugs** (Sec. 7.2) | `agent_bug_reviewer.yml` — cron **`0 3 * * *`** (diario, ~medianoche Chile) y manual                                             | Escanea `.cu/.cuh/.cpp/.h` de `nbody_2d/`. Detecta errores CUDA típicos: `cudaMalloc/cudaMemcpy/cudaFree` sin `CUDA_CHECK` (mecánico, con parche sugerido), lanzamientos `<<<>>>` sin `cudaGetLastError()` (mecánico), y **desincronizaciones host/device** en mediciones `std::chrono` sin `cudaDeviceSynchronize()` (humano). Gemini filtra falsos positivos (`REAL:`/`FALSO:`). Solo crea issues `[agent:bug]`.                                                                 |
| **Revisor de MRs** (Sec. 7.3)  | `agent_mr_reviewer.yml` — disparador **`workflow_run`** del workflow _CI — N-Body 2D_ al completar (solo eventos de PR) y manual | Comenta el PR **después** del CI con su estado (✅/❌), la verificación de issue vinculado (`Closes/Fixes/Refs #N`) y la **matriz de clasificación**: toca `.cu/.cuh` o `kernels/` → _Requiere Intervención Humana_; toca `.h` público → _Requiere Intervención Humana_; solo docs (`*.md`), `tests/` o `.github/` → _Cambio Mecánico, mergeable tras aprobación humana_. CI rojo → _No apto para merge_. Gemini entrega segunda opinión (`MECANICO`/`HUMANO`). **Nunca fusiona.** |

**Ejemplos reales y trazables de interacción:**

- El documentador detectó el placeholder de URL vacío en este README y abrió el PR de auto-fix desde la rama **`fix/docs/url-del-repositorio-vacia-en-readmemd`** (commit `c6ac0d4` _"docs(readme): fill repository URL"_ de `github-actions[bot]`).
- Desarrollo y ajustes de los propios agentes vía flujo normal de PRs: **#24** (`feat(agents)`: creación de los 3 agentes), **#44** (auto-fix PRs + etiqueta `agent:auto-fix`), **#48** (rate limit semanal) y hotfix **#45**.
- El revisor de MRs comenta los PRs de código CUDA tras el CI (ej. #70, #73, #77, #79); la clasificación esperada para PRs que tocan kernels es _"requiere revisión humana"_, comportamiento documentado en `CONTRIBUTING.md` §6.

---

## 6. Reproducibilidad de Benchmarks en Clúster DIINF (Sección 8.3)

Los benchmarks GPU se ejecutan de forma **manual** en el clúster DIINF (Xi), mediante comandos directos y jobs de Slurm lanzados por el equipo.

> **Importante:** en esta entrega **no se utilizó ningún workflow de benchmarks del DIINF** para generar las mediciones finales.

> **Ejecución final realizada (30-jul-2026):** la matriz completa se corrió en el clúster Xi del DIINF vía Slurm (job array `1309641`, 10 réplicas independientes × 10 repeticiones internas por punto, nodos `xigpu01`/`xigpu02`). Evidencia consolidada en [`results/`](results/): [`results/raw/benchmark_results.dat`](results/raw/benchmark_results.dat) (80 configuraciones), [`results/logs/cluster_run.log`](results/logs/cluster_run.log) (log maestro con metadata), [`results/figures/performance_plots.png`](results/figures/performance_plots.png) y el análisis completo en [`results/INFORME_TECNICO.md`](results/INFORME_TECNICO.md).

### 6.1 Ficha técnica del entorno de pruebas final

| Componente               | Valor                                                                                              |
| :----------------------- | :------------------------------------------------------------------------------------------------- |
| **Nodo GPU**             | NVIDIA **A30**, 24576 MiB HBM2 (nodos `xigpu01` / `xigpu02`, partición `GPU`, `--gres=gpu:A30:1`)  |
| **Driver NVIDIA**        | **580.173.02**                                                                                     |
| **CUDA Toolkit / NVCC**  | **12.1** (`V12.1.105`, en `/usr/local/cuda-12.1`)                                                  |
| **Flags de compilación** | `nvcc -O3 -std=c++17 -I. -Ikernels -Xcompiler "-Wall,-Wextra,-fopenmp"`, LDFLAGS `-lcudart`        |
| Host compiler / OS       | g++ 11.x (vía `nvcc`), Ubuntu 22.04, Slurm 22.05.2 — registrados en `results/logs/cluster_run.log` |

> **Advertencia de entorno (clúster Xi):** el symlink `/usr/local/cuda` apunta a **CUDA 10.2**, que no soporta `-std=c++17`. Es obligatorio exportar `PATH=/usr/local/cuda-12.1/bin:$PATH` y `LD_LIBRARY_PATH=/usr/local/cuda-12.1/lib64:$LD_LIBRARY_PATH` antes de compilar (detalle en `results/INFORME_TECNICO.md` §2).

### 6.2 Comandos exactos para reproducir la matriz obligatoria

El binario `nbody` es interactivo (lee por `stdin` en este orden: `seed, sys_type, N, dt, G, epsilon, steps, mode, [blockSize, gpuMethod]`). Todos los comandos se ejecutan en `nbody_2d/`:

```bash
# 0. Verificación del entorno y compilación
nvidia-smi
nvcc --version
make nbody && make benchmark

# 1. Simulación CUDA completa (modo 1): seed=42, sistema aleatorio, N=1000,
#    dt=0.01, G=1.0, eps=0.1, 500 pasos, blockSize=256, energía por reducción shared (0)
#    -> genera energy_cuda.dat y trajectories_cuda.dat
printf "42\n0\n1000\n0.01\n1.0\n0.1\n500\n1\n256\n0\n" | ./nbody 2>&1 | tee cluster_run.log

# 2. Validación de tolerancias CPU vs GPU (modo 2) -> debe imprimir [EXITO]
printf "42\n0\n1000\n0.01\n1.0\n0.1\n100\n2\n" | ./nbody 2>&1 | tee -a cluster_run.log

# 3. Matriz obligatoria de benchmarks (modo 3):
#    N ∈ {256, 512, 1024, 2000} × variantes {0 = básico, 1 = shared-memory}
#    × blockDim.x ∈ {64, 128, 256, 512, 1024} × {kernel-only, end-to-end}
#    (80 puntos de medición, 10 repeticiones c/u, baseline CPU serial, seed 42)
printf "42\n0\n1000\n0.01\n1.0\n0.1\n100\n3\n" | ./nbody 2>&1 | tee -a cluster_run.log
```

Los mismos pasos corresponden al procedimiento manual utilizado en la ejecución final y se reproducen ejecutando los comandos del clúster descritos arriba.

### 6.3 Archivos de salida generados

| Archivo                                     | Contenido                                                                                                                                                                                                                                 |
| :------------------------------------------ | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `cluster_run.log`                           | stdout completo de la sesión (vía `tee`): verificación GPU, líneas `Midiendo N=... \| Variante=... \| BlockDim=...` y resultado `[EXITO]`/`[ERROR]` de la validación.                                                                     |
| `benchmark_results.dat`                     | Resultados de la matriz GPU, 9 columnas: `N_Bodies Variant BlockDim MeasureType AvgTime(s) StdDev(s) Speedup CpuStdDev(s) SpeedupError`, con `MeasureType ∈ {kernel-only, end-to-end}` y speedup contra CPU serial (error propagado σ_S). |
| `energy_cuda.dat` / `trajectories_cuda.dat` | Series de energía K/U/E y trayectorias de la simulación CUDA (modo 1).                                                                                                                                                                    |

> **Nota sobre `scaling_analysis.dat` y `blockdim_study.dat`:** la pauta los menciona como entregables de análisis; en esta implementación **no se generan como archivos separados** — ambos análisis (escalamiento speedup vs N y estudio de `blockDim.x`) se derivan directamente de las filas `kernel-only` de `benchmark_results.dat` y son graficados por `generate_plots.py` en los paneles (a), (c) y (f) de `performance_plots.png`. El formato único evita duplicar datos y mantiene una sola fuente de verdad.

---

## 7. Pipeline de Integración Continua (CI/CD) (Sección 12)

### 7.1 Flujo principal: [`ci.yml`](.github/workflows/ci.yml) — _CI — N-Body 2D_

Disparadores: `push` y `pull_request` hacia `main`. Ejecuta en `ubuntu-latest` dentro del contenedor del proyecto:

1. `actions/checkout@v4`.
2. **Build de la imagen**: `docker build -t nbody-2d-ci -f nbody_2d/Dockerfile nbody_2d` (base `nvidia/cuda:12.4.1-devel-ubuntu22.04`).
3. **`make test`** en el contenedor (repo montado en `/workspace`): compila con `nvcc` la suite Catch2 completa (incluye los `.cu`) y la ejecuta.
4. **`make all`**: compila el binario principal `nbody`.
5. **`make benchmark-bin`**: compila el binario de benchmarks.
6. **`make benchmark`**: ejecuta benchmarks CPU y genera archivos `.dat` de baseline.
7. **Artefactos**: sube `test_runner`, `benchmark` y los `.dat` de benchmark CPU (`if: always()`, retención 7 días).

### 7.2 Desacople explícito de GPU en CI

Los runners hospedados de GitHub **no tienen GPU NVIDIA**. Por diseño: la CI **compila todo el código CUDA con `nvcc`** (garantizando que kernels, wrappers y enlazado `-lcudart` no se rompan), **ejecuta `make test` para la validación CPU/lógica** (física de referencia, integrador, métricas, tolerancias CPU) y **ejecuta benchmarks CPU** como baseline reproducible. La validación **CPU vs GPU** (modo 2) y los **benchmarks de rendimiento GPU** (modo 3) se realizan de manera dedicada en el **clúster DIINF** (Sección 6), mediante **ejecución manual en Slurm**.

### 7.3 Workflows del repositorio (`.github/workflows/`)

| Workflow                   | Disparador                             | Propósito                                                                      |
| :------------------------- | :------------------------------------- | :----------------------------------------------------------------------------- |
| `ci.yml`                   | push / PR a `main`                     | Gate obligatorio: compilación nvcc + tests CPU + benchmarks CPU en contenedor. |
| `build_base_container.yml` | push/PR que toca `nbody_2d/Dockerfile` | Build/push de la imagen base a GHCR + smoke-compile.                           |
| `release_tag.yml`          | manual                                 | Crea tag anotado `v2.0.0-lab2` + GitHub Release.                               |
| `agent_documenter.yml`     | cron semanal / push a `main` / manual  | Agente documentador (Sección 5).                                               |
| `agent_bug_reviewer.yml`   | cron diario / manual                   | Agente revisor de bugs CUDA (Sección 5).                                       |
| `agent_mr_reviewer.yml`    | `workflow_run` post-CI / manual        | Agente revisor de MRs (Sección 5).                                             |

---

## 8. Generación de Gráficos y Visualización (Sección 11)

Tras ejecutar los benchmarks en el clúster (y descargar los artefactos a `nbody_2d/`), la figura unificada del Lab 2 se genera con:

```bash
python3 scripts/generate_plots.py            # desde nbody_2d/
# equivalente: make performance-plots
# opciones: --data-dir DIR  --out RUTA_PNG  --log nbody_2d/cluster_run.log
```

- **Entradas:** `benchmark_results.dat` (matriz GPU), `energy_cuda.dat` y `trajectories_cuda.dat` (con fallback a los `.dat` CPU del Lab 1), y opcionalmente `cluster_run.log` (resumen anotado al pie de la figura).
- **Salida:** **`performance_plots.png`** (figura 2×3, 150 dpi) con: (a) speedup GPU vs N (kernel-only, mejor `blockDim`); (b) kernel-only vs end-to-end; (c) estudio de `blockDim.x` (escala log₂); (d) fracción serial estimada y límite de Amdahl; (e) trayectorias de 8 cuerpos + inset de conservación de energía; (f) kernel básico vs shared-memory.
- **Robustez:** el script nunca falla por datos faltantes — las celdas sin datos muestran _"Sin datos (pendiente ejecución en clúster DIINF)"_; dependencias: `numpy`, `pandas`, `matplotlib` (backend Agg, sin display).

Como legado del Lab 1 se conservan los scripts gnuplot (`scripts/*.gnu`): `make analysis && make plot` genera `energy_plot.png`, `trajectories_plot.png`, `speedup_plot.png`, `efficiency_plot.png` y `chunk_plot.png` a partir de los benchmarks CPU.

---

## 9. Resultados, Discusión y Limitaciones

### 9.1 Resumen de resultados GPU

Medidos en el clúster Xi DIINF (A30, job array Slurm `1309641`, 10 réplicas × 10 repeticiones internas; consolidado = media entre réplicas, σ = desv. entre réplicas). Análisis detallado en [`results/INFORME_TECNICO.md`](results/INFORME_TECNICO.md) y figuras en [`results/figures/performance_plots.png`](results/figures/performance_plots.png).

| Métrica                                                  | Resultado                                                                                                                                                                                     |
| :------------------------------------------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Validación CPU vs GPU (modo 2, `rtol=1e-4`, `atol=1e-8`) | **`[EXITO]`** — todas las aceleraciones dentro de tolerancia (N=32, seed=42, kernel básico). Además: suite Catch2 CPU **197 assertions / 50 test cases PASS** y smoke test GPU (N=3) exitoso. |
| Mejor speedup **kernel-only** (N=2000)                   | **66.2× ± 19.9** — variante shared-memory, `blockDim=128` (T_GPU = 0.666 ms vs T_CPU serial = 44.07 ms)                                                                                       |
| Mejor speedup **end-to-end** (N=2000)                    | **44.4×** — variante shared-memory, `blockDim=128`                                                                                                                                            |
| `blockDim.x` óptimo observado                            | **128** en prácticamente toda la matriz; `blockDim=1024` degrada ~2.4× (menor ocupación de SMs y tile shared de 24 KB)                                                                        |
| Ganancia shared-memory vs básica                         | **~1.2× estable** (1.19×–1.22× kernel-only), independiente de N — consistente con mitigación constante del cuello de botella de memoria global                                                |
| Deriva de energía en simulación CUDA                     | **−0.072 %** en 5000 pasos (oscilación 0.55 %; showcase `diskSystem` N=200, dt=0.001, ε=0.1, t=5.0)                                                                                           |

**Escalamiento con N (kernel-only, mejor configuración):** 7.4× (N=256) → 16.0× (N=512) → 33.2× (N=1024) → **66.2×** (N=2000). El speedup end-to-end converge al kernel-only a medida que N crece porque el overhead de transferencias H2D/D2H es ~constante (0.27–0.31 ms) mientras el kernel crece O(N²): pesa 70 % del paso en N=256 pero solo 28 % en N=2000. La predicción de Amdahl con fracción serial decreciente f_s(N) reproduce los speedups e2e medidos con **error < 2 %**.

> **Nota sobre el showcase físico:** una primera corrida con `randomSystem` (N=1000, dt=0.01, ε=0.01) derivó +2131 % en energía (sistema no virializado → colapso con _slingshots_ numéricos); se re-parametrizó a disco con dt=0.001/ε=0.1 obteniendo la conservación reportada. Los datos originales quedaron archivados sin modificar en `results/raw/legacy_random_dt0.01/` y el análisis del incidente está en `results/INFORME_TECNICO.md` §7.

### 9.2 Discusión: Ley de Amdahl y fracción serial

La simulación CUDA calcula las **fuerzas en GPU**, pero la **integración de Euler (kick/drift) se ejecuta en el host** (`Integrator::integrateEulerGpu`): por cada paso se transfieren aceleraciones D2H y posiciones/velocidades H2D. Esa transferencia + integración CPU constituye una **fracción serial** $f_s$ estimable desde los propios benchmarks como $f_s \approx (T_{e2e} - T_{kernel}) / T_{e2e}$, que acota el speedup extremo a $1/f_s$ según Amdahl (panel (d)). El diseño permite cuantificar exactamente ese costo al medir por separado _kernel-only_ (buffer fuera del timer) y _end-to-end_ (H2D + kernel + D2H), ambos con `std::chrono::steady_clock` en host y `cudaDeviceSynchronize()` antes de detener el reloj.

**Medido en el clúster:** $f_s$ **decrece con N** — 0.70 (N=256) → 0.57 (N=512) → 0.41 (N=1024) → 0.28 (N=2000) — porque el overhead de transferencias es ~constante (0.27–0.31 ms) y el kernel crece O(N²). Por tanto no existe un techo único de Amdahl: la "asíntota" $1/f_s$ crece con N y la GPU aprovecha mejor los problemas grandes. El modelo `S_e2e = S_kernel / (1 + f_s·(S_kernel − 1))` reproduce las mediciones con **error < 2 %** (detalle en `results/INFORME_TECNICO.md` §6.5).

### 9.3 Supuestos y límites (hardware/modelo)

1. **Algoritmo All-Pairs O(N²):** sin poda jerárquica tipo Barnes-Hut; el cómputo crece cuadráticamente y para N muy grandes dominaría incluso con GPU ideal.
2. **Sincronización por paso:** la integración en host obliga a H2D/D2H por paso temporal; es el principal limitante de escala (ver §9.2), no el kernel.
3. **Medición sin `cudaEvent`:** los tiempos GPU se miden con `std::chrono::steady_clock` + `cudaDeviceSynchronize()` (decisión documentada en `CHANGELOG.md`), con overhead de sincronización incluido de forma conservadora y homogénea entre variantes.
4. **Sin flags `-arch` específicos:** `nvcc` compila para la arquitectura por defecto; incluye parche `atomicAdd(double)` vía CAS para `__CUDA_ARCH__ < 600`.
5. **Benchmarks atados al hardware:** los números finales solo son válidos para el nodo GPU del clúster DIINF donde se ejecuten (ficha §6.1); la CI garantiza compilación y corrección lógica, no rendimiento.
6. **Modelo físico:** unidades adimensionales (`G=1`), integrador Euler explícito (`dt=0.01`) y suavizado `epsilon` para evitar divergencias a distancia ~0; la validación de corrección se ancla al baseline CPU serial con semilla fija 42.
