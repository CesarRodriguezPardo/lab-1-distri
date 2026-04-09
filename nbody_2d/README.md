# Laboratorio 1: Simulador Gravitatorio N-Cuerpos 2D

**Curso:** Sistemas Distribuidos  
**Universidad:** Universidad de Santiago de Chile  
**Profesor:** Miguel Cárcamo | **Ayudante:** Diego Fernández  
**Entrega:** 8 de mayo de 2026

---

## Equipo

| Nombre | Rol | Responsabilidades |
|--------|-----|-------------------|
| (Integrante 1) | Modelo y datos | `Particle`, `NBodySystem`: masas, posiciones, velocidades, I/O de estados |
| (Integrante 2) | Núcleo paralelo | `NBodySystem::computeAccelerations*`: bucles todo-pares, OpenMP, schedules, collapse |
| (Integrante 3) | Integración y física | `Integrator`, `NBodySimulator`: Euler, criterios de estabilidad, energía |
| (Integrante 4) | Métricas y benchmarks | `MetricsCalculator`, `Benchmark`: speedup, eficiencia, Amdahl, propagación de errores |
| (Integrante 5) | Calidad, CI y visualización | Tests, Docker, pipeline CI, `Visualizer`, scripts Python/Gnuplot |

---

## Sistema de unidades (adimensional)

Todas las simulaciones usan un **sistema adimensional coherente**:

| Magnitud | Unidad | Valor de referencia |
|----------|--------|---------------------|
| Masa     | M      | masa característica del sistema = 1 |
| Longitud | L      | separación característica = 1 |
| Tiempo   | T      | T = sqrt(L³ / (G·M)) |
| G        | —      | **G = 1.0** (fijo en todo el repositorio) |

> Cualquier resultado expresado en estas unidades puede escalar a un sistema físico
> real multiplicando por las constantes adecuadas. Los benchmarks y verificaciones
> usan siempre estas unidades salvo indicación explícita.

---

## Parámetros por defecto

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| G | 1.0 | Constante gravitacional (sistema adimensional) |
| ε (epsilon) | 0.10 | Suavizado de Plummer (evita singularidades) |
| Δt | 0.001 | Paso temporal (estabilidad verificada para N ≤ 2000) |
| N | 500 – 2000 | Número de cuerpos |
| Pasos | 1000 | Número de pasos temporales por defecto |
| Semilla | 42 | Semilla para reproducibilidad de condiciones iniciales |

**Criterio de tolerancia en comparaciones de punto flotante:**  
Los tests usan tolerancia absoluta `tol = 1e-10` salvo en casos con masas grandes
o cantidades acumuladas, donde se usa tolerancia relativa `|diff| / |valor| < 1e-12`.

---

## Compilación

### Requisitos

- `g++ >= 9` con soporte para C++17 y OpenMP (`-fopenmp`)
- `make`
- (Opcional) Python 3 + Matplotlib para gráficos

### Build principal

```bash
make all
```

Genera el binario `./nbody_2d`.

### Compilación manual

```bash
g++ -Wall -Wextra -O3 -fopenmp -std=c++17 \
    -o nbody_2d \
    main.cpp Particle.cpp NBodySystem.cpp NBodySimulator.cpp \
    Integrator.cpp MetricsCalculator.cpp Benchmark.cpp Visualizer.cpp \
    -lgomp
```

---

## Ejecución

```bash
# Verificación física (N=2, N=3) — Semana 1
./nbody_2d

# Simulación completa (Semana 3)
./nbody_2d --simulate

# Benchmarks OpenMP (Semana 4)
./nbody_2d --benchmark

# Análisis de escalabilidad (Semana 4)
./nbody_2d --analysis

# Ayuda
./nbody_2d --help
```

---

## Pruebas

```bash
make test
```

Compila y ejecuta `tests/test_physics.cpp` con las pruebas de la Semana 1:

- Creación de partículas, kick, drift, zeroAcceleration
- Aceleración gravitatoria N=2 (con y sin suavizado)
- Ejemplo numérico del enunciado (PDF §4.2): `a₁ₓ ≈ 0.9852`
- Ley de acción-reacción de Newton: `F_ij = -F_ji`
- Superposición N=3 verificada analíticamente a mano
- Conservación de momento lineal
- Guardas de error: masa negativa, ε negativo, G negativo

---

## Benchmarks

```bash
make benchmark
```

*(Implementado en Semana 4)* Genera:

- `benchmark_results.dat` — tiempos para `schedule(static/dynamic/guided)` vs chunk
- `scaling_analysis.dat` — speedup S_p y eficiencia E_p vs número de hilos

---

## Análisis

```bash
make analysis
```

*(Implementado en Semana 4)* Genera las figuras PNG requeridas y calcula la
fracción serial `f` para la predicción de Amdahl.

---

## Docker

```bash
# Construir imagen
docker build -t nbody2d .

# Correr tests dentro del contenedor
docker run --rm nbody2d make test

# Correr la verificación de Semana 1
docker run --rm nbody2d ./nbody_2d

# Benchmarks (con n hilos disponibles)
docker run --rm nbody2d make benchmark
```

---

## Cómo repetir los experimentos

1. Clonar el repositorio y entrar al directorio:
   ```bash
   git clone <URL>
   cd lab-1-distri/nbody_2d
   ```

2. Compilar:
   ```bash
   make all
   ```

3. Verificación Semana 1 (N=2, N=3):
   ```bash
   ./nbody_2d
   ```

4. Tests automáticos:
   ```bash
   make test
   ```

5. Benchmarks completos (Semana 4, repetir mínimo 10 veces automáticamente):
   ```bash
   make benchmark
   make analysis
   ```

6. Limpiar artefactos:
   ```bash
   make clean
   ```

**Hardware de referencia para benchmarks:**
- CPU: (completar con la máquina del equipo, ej. Intel Core i7-12700, 12 núcleos)
- RAM: (completar, ej. 16 GB DDR4)
- OS: (completar, ej. Ubuntu 22.04 LTS)
- Compilador: `g++ --version` → (completar)
- Flags: `-Wall -Wextra -O3 -fopenmp -std=c++17`

---

## Estructura del proyecto

```
nbody_2d/
├── main.cpp               # Punto de entrada y verificaciones Semana 1
├── Particle.h / .cpp      # Estado 2D de cada cuerpo (masa, pos, vel, acc)
├── NBodySystem.h / .cpp   # Contenedor; computeAccelerations* (serial + OpenMP)
├── NBodySimulator.h / .cpp# Lazo temporal; variantes OpenMP de integración
├── Integrator.h / .cpp    # Paso de Euler (y opcionalmente Verlet/leapfrog)
├── MetricsCalculator.h/.cpp # K, U, momento, R_cm, R_rms, d_min
├── Benchmark.h / .cpp     # Medición de tiempos, speedup, eficiencia, Amdahl
├── Visualizer.h / .cpp    # Exportación de trayectorias y snapshots
├── tests/
│   └── test_physics.cpp   # Suite de tests unitarios e integración (Semana 1+)
├── Makefile
├── Dockerfile
└── README.md
```

---

## Cláusulas OpenMP implementadas

| Cláusula | Dónde | Método |
|----------|-------|--------|
| `schedule(static, chunk)` | `NBodySystem` | `computeAccelerations(0, chunk)` |
| `schedule(dynamic, chunk)` | `NBodySystem` | `computeAccelerations(1, chunk)` |
| `schedule(guided, chunk)` | `NBodySystem` | `computeAccelerations(2, chunk)` |
| `collapse` | `NBodySystem` | `computeAccelerationsCollapse()` |
| `atomic` | `NBodySimulator` | `integrateEuler(0)` |
| `critical` | `NBodySimulator` | `integrateEuler(1)` |
| `nowait` | `NBodySimulator` | `integrateEuler(2)` |
| `reduction` | `NBodySimulator` | `calculateEnergy(0)` |
| `barrier` | `NBodySimulator` | `simulatePhasesBarrier()` |
| `single` | `NBodySimulator` | `parallelInitializationSingle()` |
| `task` | `NBodySimulator` | `processBodies(0)` |
| `firstprivate` | `NBodySimulator` | `calculateMetricsFirstprivate()` |
| `lastprivate` | `NBodySimulator` | `calculateFinalStateLastprivate()` |
| `private`, `shared` | todas | en todo bucle paralelo relevante |

*(Las celdas vacías se completarán durante las semanas correspondientes)*

---

## Calendario de desarrollo

| Semana | Hitos |
|--------|-------|
| 1 | Repositorio, Docker, CI mínimo; `Particle` + `NBodySystem` serial; verificación N=2, N=3; tests unitarios de distancia y aceleración |
| 2 | `computeAccelerations` paralelo (schedules, collapse); comparación serial vs paralelo; tolerancia de punto flotante justificada |
| 3 | `NBodySimulator` + `Integrator`; lazo temporal completo; K, U, momento; pruebas de integración |
| 4 | `Benchmark`; experimentos de escalabilidad; gráficos con barras de error; ley de Amdahl |
| 5 | CI estable con `make test`; contenedor Docker pulido; reporte PDF; revisión cruzada |

---

## URL del repositorio

```
https://github.com/<org-o-usuario>/<repo>   ← completar antes de la entrega
```

> El cuerpo docente necesita acceso para revisar CI/CD y los resultados de
> `make test`. Si el repositorio es privado, invitar al profesor y al ayudante
> como colaboradores con permisos de lectura de pipelines.