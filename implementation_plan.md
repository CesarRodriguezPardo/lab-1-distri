# Todo lo que falta implementar — Lab 1 N-Body (Rol CI)

> Estado al 05-May-2026.
> Completados esta semana: test `zeroAccelerations`, `plot_speedup.gnu`, `plot_efficiency.gnu`, Makefile simplificado.

---

## Módulo 1 — Tests: `MetricsCalculator` 🔴

**Archivo nuevo:** `tests/test_MetricsCalculator.cpp`  
**Agregar a Makefile TEST_SRCS:** `tests/test_MetricsCalculator.cpp`

La clase tiene lógica física sustantiva y **cero tests**. El lab exige cobertura de toda clase pública con lógica.

---

### TEST MC-1 · `calculateEnergy(method=0)` — energía potencial y K=0

```
Sistema: 2 cuerpos en reposo
  p0: masa=1, pos=(0,0), vel=(0,0)
  p1: masa=1, pos=(3,4), vel=(0,0)
  G=1.0, eps=0.0

Analítico:
  r = sqrt(3²+4²) = 5.0
  K = 0.0
  U = -G·m0·m1 / r = -1/5 = -0.2
  E = -0.2

Verificar: getKineticEnergy(), getPotentialEnergy(), getTotalEnergy()
Tolerancia: margin(1e-10)
```

---

### TEST MC-2 · `calculateEnergy(method=0)` — energía cinética pura

```
Sistema: 1 cuerpo aislado (sin pares → U=0)
  p0: masa=2.0, pos=(0,0), vel=(3,4)
  G=1.0, eps=0.0

Analítico:
  K = 0.5 · 2 · (3²+4²) = 0.5 · 2 · 25 = 25.0
  U = 0.0  (no hay pares i<j)
  E = 25.0

Verificar: getKineticEnergy(), getTotalEnergy()
```

---

### TEST MC-3 · `calculateEnergy(method=1)` — consistencia con method=0

```
Mismo sistema que MC-1.
Correr calculateEnergy(0) → guardar K0, U0.
Correr calculateEnergy(1) → guardar K1, U1.
Verificar: K1 == K0, U1 == U0  (margin 1e-10)

Detecta: errores en el padding anti-false-sharing que produzcan resultados distintos.
```

---

### TEST MC-4 · `calculateMetricsFirstprivate()` — centro de masa

```
Sistema: 2 cuerpos de masa igual en reposo
  p0: masa=1, pos=(0,0), vel=(0,0)
  p1: masa=1, pos=(10,0), vel=(0,0)

Analítico:
  CM_x = (1·0 + 1·10)/(1+1) = 5.0
  CM_y = 0.0

Verificar: getCmX(), getCmY()
```

---

### TEST MC-5 · `calculateMetricsFirstprivate()` — momentum total nulo

```
Sistema: 2 cuerpos con momentum opuesto
  p0: masa=2, vel=(3,0)
  p1: masa=1, vel=(-6,0)

Analítico:
  px = 2·3 + 1·(-6) = 0
  py = 0
  momentumMagnitude = 0.0

Verificar: getMomentumMagnitude()
Detecta: si firstprivate inicializa mal las variables de acumulación.
```

---

### TEST MC-6 · `calculateFinalStateLastprivate()` — RMS radius

```
Requiere: calcular CM primero (llamar a calculateMetricsFirstprivate())
Sistema: 4 cuerpos de masa=1 en cuadrado simétrico centrado en origen
  p0=(1,1), p1=(-1,1), p2=(-1,-1), p3=(1,-1)

Analítico:
  CM = (0,0)
  sumSqDist = 4 · (1²+1²) = 8.0
  totalMass = 4.0
  rmsRadius = sqrt(8/4) = sqrt(2) ≈ 1.41421356...

Verificar: getRmsRadius()  margin(1e-10)
```

---

### TEST MC-7 · `calculateFinalStateLastprivate()` — distancia mínima

```
Sistema: 3 cuerpos colineales
  p0=(0,0), p1=(3,0), p2=(10,0)

Distancias inter-cuerpo:
  d(0,1) = 3.0
  d(0,2) = 10.0
  d(1,2) = 7.0
  minDistance = 3.0

Verificar: getMinDistance()
```

---

## Módulo 2 — Tests: `NBodySimulator` versiones paralelas 🟡

**Archivo:** `tests/test_NBodySystem.cpp` (agregar al final)

---

### TEST SIM-1 · `integrateEuler(syncType=1)` vs serial

```
Sistema: N=5, seed=42, dt=0.01, steps=10
Correr 10 pasos con integrateEuler() serial → guardar posiciones.
Correr 10 pasos con integrateEuler(1) critical → comparar.

Verificar: posiciones idénticas  margin(1e-14)
Detecta: race conditions en la versión critical.
```

---

### TEST SIM-2 · `integrateEuler(syncType=2)` vs serial

```
Mismo escenario que SIM-1 pero con syncType=2 (nowait).
```

---

## Módulo 3 — Benchmark: Tiempo vs. Chunk × Schedule 🔴

> Este módulo también genera `benchmark_results.dat`, que el lab exige explícitamente.

---

### 3.1 — Nueva estructura en `Benchmark.h`

```cpp
struct ChunkResult {
    int    scheduleType;   // 1=static, 2=dynamic, 3=guided
    int    chunkSize;
    double avgTime;
    double stdDevTime;
};
```

Agregar a clase `Benchmark`:
- Vector `chunkResults` de tipo `std::vector<ChunkResult>`
- Método `runChunkAnalysis(int numThreads, std::vector<int> chunkSizes, std::vector<int> schedules, const std::function<void(int,int)>& func)`
- Método `saveChunkResultsToFile(const std::string& filename)`

---

### 3.2 — Implementación en `Benchmark.cpp`

`runChunkAnalysis` itera sobre `schedules × chunkSizes`, llama a `runExperiment` para cada combinación con hilos fijos, y llena `chunkResults`. El archivo de salida tiene formato:

```
Schedule ChunkSize AvgTime StdDevTime
static   1         0.00021 0.00003
static   4         0.00019 0.00002
...
```

---

### 3.3 — Extender `benchmark_main.cpp`

Agregar un segundo bloque después del análisis de escalabilidad:

```cpp
// Parámetros para chunk analysis
int fixedThreads = 4;
std::vector<int> chunkSizes  = {1, 4, 16, 64, 256};
std::vector<int> schedules   = {1, 2, 3};  // static, dynamic, guided

Benchmark benchChunk(20);
benchChunk.runChunkAnalysis(fixedThreads, chunkSizes, schedules,
    [&](int sched, int chunk) {
        #pragma omp single
        system.zeroAccelerations();
        system.computeAccelerations(sched, chunk, true);
    });
benchChunk.saveChunkResultsToFile("benchmark_results.dat");
```

---

### 3.4 — Script Gnuplot `scripts/plot_chunk.gnu`

**Entrada:** `benchmark_results.dat`  
**Salida:** `chunk_plot.png`

Tres líneas (una por schedule) en un gráfico semilogarítmico en X (chunk size):
- Eje X: chunk size (1, 4, 16, 64, 256) — escala log₂
- Eje Y: tiempo promedio T [s] con barras de error
- Una línea por schedule (static=naranja, dynamic=azul, guided=verde)

---

### 3.5 — Actualizar `make plot` en Makefile

Agregar la llamada al nuevo script:
```makefile
gnuplot $(SCRIPTS_DIR)/plot_chunk.gnu
```
Y agregar `chunk_plot.png` a la lista de salidas.

---

## Resumen de archivos afectados

| Archivo | Operación | Módulo |
|---------|-----------|--------|
| `tests/test_MetricsCalculator.cpp` | **CREAR** (nuevo) | 1 |
| `Makefile` (TEST_SRCS) | **MODIFICAR** | 1 |
| `tests/test_NBodySystem.cpp` | **AÑADIR** al final | 2 |
| `Benchmark.h` | **MODIFICAR** (nueva API) | 3 |
| `Benchmark.cpp` | **MODIFICAR** (nueva impl.) | 3 |
| `benchmark_main.cpp` | **MODIFICAR** (2° bloque) | 3 |
| `scripts/plot_chunk.gnu` | **CREAR** (nuevo) | 3 |
| `Makefile` (plot target) | **MODIFICAR** | 3 |

---

## Dependencias de implementación

```
Módulo 1 (MC tests) ──────────────────────────────────────▶ make test ✓
Módulo 2 (SIM tests) ─────────────────────────────────────▶ make test ✓
Módulo 3.1 (Benchmark.h) ──▶ 3.2 (Benchmark.cpp)
                                      │
                      ┌───────────────┘
                      ▼
               3.3 (benchmark_main.cpp) ──▶ benchmark_results.dat
                                                    │
                                                    ▼
                              3.4 (plot_chunk.gnu) ──▶ chunk_plot.png
                                                    │
                              3.5 (Makefile) ───────┘
```

Módulos 1 y 2 son independientes entre sí y de Módulo 3.
