# Guía para el equipo — Laboratorio 2 (N-Body CUDA)

Este documento explica **qué debes hacer**, **dónde** y **cómo**, según tu rol en el
Laboratorio 2 de Sistemas Distribuidos.

## 1. Antes de empezar

Asegúrate de que Maxito te haya agregado como colaborador al repositorio
(necesitas acceso de **Write** para crear ramas y PRs).

Clona el repositorio y ubícate en `main`:

```bash
git clone git@github.com:CesarRodriguezPardo/lab-1-distri.git
cd lab-1-distri
git checkout main
git pull origin main
```

## 2. Tu rama

**Nunca trabajes directamente en `main`.** `main` está protegida: no acepta
push directo y requiere PR + revisión humana + CI verde.

Crea una rama con el prefijo `feature/`:

```bash
git checkout -b feature/<nombre-descriptivo>
```

Ejemplos:
- `feature/kernel-basico`
- `feature/buffer-sao`
- `feature/integracion-cpu-gpu`
- `feature/docker-cuda`

## 3. Tu issue y dónde trabajar

Cada rol tiene un issue asignado. El issue es tu **tarea oficial**; el PR
que abras debe cerrarlo con `Closes #NN` en su descripción.

### Rol 1 — Kernels CUDA (Sofía)

| Dato | Valor |
|---|---|
| Issue | **#20** — `feat(cuda): implementar kernel computeAccelerations básico y shared-memory` |
| Archivos | `nbody_2d/kernels/accelerations.cu`, `nbody_2d/kernels/accelerations.cuh`, `nbody_2d/NBodySystem.cpp`, `nbody_2d/NBodySystem.h` |

**Qué debes hacer:**

- Implementar `computeAccelerationsKernel` básico: un hilo CUDA por cuerpo `i`,
  cada hilo recorre todos los `j ≠ i` en serie. Mapeo 1D: `i = blockIdx.x * blockDim.x + threadIdx.x`.
- Implementar `computeAccelerationsKernelShared`: tiles de posiciones (y masas si aplica)
  cargados en `__shared__`, con `__syncthreads()` entre carga y uso.
- Agregar lanzadores host en `NBodySystem` con sobrecarga: `computeAccelerationsGpu()`,
  `computeAccelerationsGpu(int variant)`, `computeAccelerationsGpu(int variant, int blockSize)`.
- Usar la macro `CUDA_CHECK` tras cada llamada a la API CUDA y `cudaGetLastError()`
  tras cada lanzamiento de kernel.
- Proteger bordes: si `i >= N`, el hilo no hace nada.

### Rol 2 — Host/device y SoA (Martín)

| Dato | Valor |
|---|---|
| Issue | **#21** — `feat(cuda): implementar buffer host/device y layout SoA` |
| Archivos | `nbody_2d/CudaBuffer.h`, `nbody_2d/NBodySystem.cpp`, `nbody_2d/NBodySystem.h`, `nbody_2d/NBodySimulator.cpp` |

**Qué debes hacer:**

- Implementar `CudaBuffer` con RAII: constructor que llama a `cudaMalloc`,
  destructor que llama a `cudaFree`, prohibir copia (o implementar movimiento).
- Layout SoA en device: `d_mass`, `d_x`, `d_y`, `d_vx`, `d_vy` (arreglos separados
  en vez de arreglo de structs) para favorecer coalescing.
- Minimizar `cudaMemcpy` por paso temporal: mantener el estado en device entre
  pasos, solo copiar a host cuando sea necesario (lectura de resultados).
- `cudaDeviceSynchronize()` donde corresponda para garantizar que los kernels
  terminaron antes de leer resultados.

### Rol 3 — Integración y validación (Nicolás)

| Dato | Valor |
|---|---|
| Issue | **#22** — `feat(testing): integrar Euler host-device y validar CPU vs GPU` |
| Archivos | `nbody_2d/NBodySimulator.cpp`, `nbody_2d/NBodySimulator.h`, `nbody_2d/Integrator.cpp`, `nbody_2d/Integrator.h`, `nbody_2d/MetricsCalculator.cpp`, `nbody_2d/tests/test_NBodySystem.cpp` |

**Qué debes hacer:**

- Integración Euler en host tras sincronizar el device: orden fijo del paso temporal:
  1) lanzar kernel de aceleraciones,
  2) `cudaDeviceSynchronize()`,
  3) actualizar velocidades y posiciones en host (Euler explícito),
  4) copiar a device solo si el siguiente paso lo requiere.
- Implementar `stepEulerGpu()`, `calculateEnergyGpu()` (reducción en shared memory
  y variante con `atomicAdd`).
- Tests CPU vs GPU: comparar aceleraciones con tolerancias `rtol = 1e-4`,
  `atol = 1e-8`. La referencia CPU serial del Lab 1 es la fuente de verdad.
- La función `compareCpuGpu(int nBodies)` en `Benchmark` debe validar la equivalencia.
- Documenta en el README los valores de tolerancia y por qué se eligieron.

### Rol 5 — CI, Docker y visualización (Sebastián)

| Dato | Valor |
|---|---|
| Issue | **#23** — `feat(ci): CI CUDA, Docker y visualización de benchmarks` |
| Archivos | `.github/workflows/`, `nbody_2d/Dockerfile`, `nbody_2d/Makefile`, `nbody_2d/scripts/`, `nbody_2d/Benchmark.cpp/.h` |

**Qué debes hacer:**

- Extender el CI actual para que compile código CUDA (nvcc) y ejecute `make test`
  en cada PR. Opcional: agregar un job con GPU en CI.
- Dockerfile con imagen CUDA: `nvidia/cuda:12.x-devel-ubuntu22.04`. El contenedor
  debe compilar y ejecutar `make test`.
- Métricas de benchmark: `benchmarkKernelOnly()` y `benchmarkEndToEnd()` con
  `std::chrono::steady_clock` alrededor de `cudaDeviceSynchronize()` (sin `cudaEvent`).
- Estudio de `blockDim.x`: probar 64, 128, 256, 512, 1024 para ambas variantes.
- Scripts de Gnuplot para: speedup GPU vs CPU, tiempo vs blockDim.x, curva de
  Amdahl, trayectorias, energía E(t).
- Los benchmarks finales deben correr en el nodo GPU del clúster DIINF (documentar
  nodo, GPU, driver, flags de compilación, y comando exacto).

## 4. Cómo hacer un commit

Usa **commits convencionales** con el prefijo que corresponda a tu rol:

```bash
git add <archivos>
git commit -m "feat(cuda): <descripción corta de lo que hiciste>"
git push -u origin feature/<nombre-de-tu-rama>
```

Prefijos según tu rol:
- Rol 1 (kernels): `feat(cuda):` o `fix(cuda):`
- Rol 2 (memoria): `feat(memory):` o `fix(memory):`
- Rol 3 (testing): `feat(testing):` o `fix(testing):`
- Rol 5 (CI): `feat(ci):` o `fix(ci):`

Ejemplos:
- `feat(cuda): implement shared-memory tile kernel for accelerations`
- `feat(memory): add CudaBuffer RAII wrapper with SoA layout`
- `feat(testing): add CPU vs GPU equivalence test for N=3`
- `feat(ci): extend CI pipeline with CUDA compilation`

## 5. Cómo crear tu Pull Request

1. Después de hacer `git push`, GitHub te dará un link para crear el PR
   (también puedes ir a la pestaña **Pull requests** → **New pull request**).

2. **Base:** `main` ← **Compare:** tu rama.

3. **Título:** igual que tu commit principal, ej: `feat(cuda): implement shared-memory kernel`.

4. **Descripción:** explica qué hiciste y termina con:
   ```
   Closes #NN
   ```
   (reemplaza `NN` por tu número de issue: #20, #21, #22 o #23).

5. **Reviewers:** asigna a cualquier compañero del equipo.

6. Haz clic en **Create pull request**.

## 6. Qué pasa después de crear el PR

1. **CI corre automáticamente** (Build & Test in container). Si falla, arregla el error
   y haz push de nuevo.

2. **Un agente de IA comenta en tu PR** con una clasificación (normalmente dirá
   "requiere revisión humana" porque tu código toca kernels/física — es correcto y esperado).

3. **Un compañero revisa y aprueba.** Sin aprobación no se puede fusionar.

4. Cuando CI esté en verde ✅ y tengas 1 aprobación, Maxito (o tú si tienes write
   access) hace clic en **Merge pull request**. La rama se borrará automáticamente.

5. El issue se cerrará solo al fusionar el PR (gracias al `Closes #NN`).

## 7. Reglas importantes

| Regla | Motivo |
|---|---|
| No pushear a `main` | Está protegida; todo cambio pasa por PR |
| `Closes #NN` en cada PR | El issue se cierra solo; trazabilidad obligatoria (se descuenta en nota si no) |
| 1 aprobación humana mínima | Revisión cruzada del equipo |
| CI verde obligatorio | `make test` debe pasar antes de fusionar |
| Commits con prefijo (`feat:`, `fix:`) | Historial profesional y legible |
| NO tocar `.github/`, `CHANGELOG.md` ni `README.md` | Ya están configurados; el README lo actualiza el equipo al final |

## 8. Cómo probar localmente

Dentro de `nbody_2d/`:

```bash
make clean
make all          # compilar
make test         # correr todos los tests unitarios
make benchmark    # benchmarks de rendimiento
```

Asegúrate de que `make test` pase antes de hacer push. Si no compila en tu
máquina, no compilará en CI.

## 9. Dudas

Cualquier duda técnica sobre tu rol, pregúntale a Maxito o revisa el enunciado
del Lab 2 (secciones 3, 4, 5, 6, 8, 9, 10, 11, 12 y 13).
