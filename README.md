# Laboratorio 2: Simulación N-Body 2D con CUDA

Este repositorio contiene una simulación computacional del problema de los N-Cuerpos en 2D, paralelizada utilizando OpenMP y validada rigurosamente mediante el framework de pruebas Catch2.

## 1. Información del Equipo y Roles

### Tabla de Integrantes

| Nombre                | Rol | Responsabilidades Principales                                                                                                                           |
| :-------------------- | :-: | :------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Sofía Gacitúa**     |  1  | Coordinación general, diseño inicial de la arquitectura orientada a objetos e implementación de los sistemas de partículas, también responsable de I/O. |
| **Martín Salinas**    |  2  | Diseño e implementación de paralelización, utilizando distintas cláusulas de OpenMP solicitadas.                                                        |
| **Nicolás García**    |  3  | Desarrollo serial del sistema y encargado de implementar las físicas y el integrador Euler.                                                             |
| **Sebastián Cassone** |  4  | Desarrollo del módulo de métricas (`MetricsCalculator`), y benchmark para implementación de mediciones.                                                 |
| **César Rodríguez**   |  4  | Git, releases y agentes: protección de `main`, `CHANGELOG.md`, issues etiquetados, tres agentes de IA en CI, release `v2.0.0-lab2`. |

### Hitos del Proyecto

- **Hito 1 (Semana 1):** Setup de entorno, definición orientada a objetos (Particle, NBodySystem) y simulación puramente matemática serial.
- **Hito 2 (Semana 2):** Paralelización del algoritmo $O(N^2)$ usando OpenMP, enfrentando mitigaciones de concurrencia y particionamiento espacial.
- **Hito 3 (Semana 3):** Desarrollo profundo del Testing Framework (Catch2), solución de condiciones de carrera (Race Conditions) e Integración Continua.
- **Hito 4 (Semana 4):** Benchmarking final del desempeño de hilos, creación de reportes gráficos y consolidación de la documentación.

### Flujo de trabajo Git

- `main` protegida: sin push directo, merge solo vía Pull Request.
- 1 revisión humana + CI verde requeridos para fusionar.
- Ramas: `feature/<nombre>` y `fix/<nombre>`, eliminadas automáticamente al fusionar.
- Commits convencionales: `feat(scope):`, `fix(scope):`, `docs(scope):`.
- Todo PR cierra un issue: `Closes #N`.
- Documentación completa en [`.github/GIT_FLOW.md`](.github/GIT_FLOW.md).

## Agentes de IA

Tres agentes automatizan documentación, revisión de bugs y revisión de PRs.
Scripts en [`.github/agents/`](.github/agents/), disparadores en
[`.github/workflows/`](.github/workflows/).

| Agente | Herramienta | Frecuencia | Criterio mecánico | Criterio humano |
|---|---|---|---|---|
| Documentador | Python + Gemini 2.5 Flash | Semanal (lunes) + push a `main` | Typo, enlace vacío, sección faltante → abre PR con fix (`agent:auto-fix`) | Decisiones de diseño, explicar kernels → issue "Requiere intervención humana" |
| Revisor de bugs | Python + Gemini 2.5 Flash | Diario (cron 03:00 UTC) | Falta `CUDA_CHECK`, kernel sin `cudaGetLastError` → issue con parche | Afecta física o API pública → issue |
| Revisor de MRs | Python + Gemini 2.5 Flash | Al terminar CI en un PR (`workflow_run`) | Solo docs/tests, CI verde, issue vinculado → "Mecánico y mergeable" | Cambios en kernels o física → "Requiere revisión humana"; si CI falla lo dice |

- Motor: **Gemini 2.5 Flash** con API key en GitHub Secrets.
- Máximo 5 issues automáticos por agente por semana.
- **Nunca** fusionan a `main` sin aprobación humana.

---

## 2. Descripción Técnica y Arquitectura

### Documentación de Clases

La arquitectura del software está fuertemente desacoplada y orientada a objetos:

- **`Particle`**: Entidad fundamental. Encapsula masa, posición, velocidad y aceleración de un cuerpo en el plano bidimensional. Facilita la aplicación del integrador temporal (`kick` y `drift`).
- **`NBodySystem`**: Coordina el universo de las partículas. Posee la lógica dura (física) para resolver la sumatoria anidada de interacción gravitacional entre pares de partículas.
- **`Integrator`**: Capa lógica encargada de resolver las ecuaciones de movimiento temporales bajo el paradigma de Euler (simple). Contiene primitivas para ejecutarse de modo serial y en paralelo (usando `critical`, `atomic` o `nowait`).
- **`NBodySimulator`**: Orquestador principal que une el paso temporal (tick) mediante la petición conjunta de `computeAccelerations` a `NBodySystem` y la aplicación del avance temporal delegada al integrador.
- **`MetricsCalculator`**: Herramienta analítica y de instrumentación pasiva. Mide el comportamiento físico del sistema (Energía cinética/potencial y sus preservaciones, centro de masa).

### Mapeo de Cláusulas OpenMP

El código hace uso de una amplia gama de pragmas obligatorios:

- **`#pragma omp parallel for schedule(static/dynamic)`**: Localizados en `NBodySystem.cpp` (`computeAccelerations`) y `Benchmark.cpp` para distribuir ciclos iterativos For.
- **`#pragma omp atomic`**: Ubicado en `Integrator.cpp` y `NBodySystem.cpp` para resolver la acumulación de variables cruzadas en memoria compartida, previniendo _data races_.
- **`#pragma omp critical`**: Presente en `Integrator.cpp` (variante 1) para demostrar bloqueos mutuos seguros como contraste educativo.
- **`#pragma omp for nowait`**: Empleado en `Integrator.cpp` (variante 2) para eludir la barrera sintética implícita tras el final de un bucle For restrictivo.
- **`reduction`, `firstprivate`, `lastprivate`**: Aplicados y demostrados en `MetricsCalculator.cpp` al efectuar sumatorias compartidas (como la reducción agregada de la Energía Total a partir de pasos intermedios de los hilos de trabajador).

**URL del Repositorio:** https://github.com/CesarRodriguezPardo/lab-1-distri

---

## 3. Especificaciones Físicas y Unidades

### Sistema de Unidades

La simulación utiliza un **sistema de unidades adimensional natural**. Ajustamos la constante de Gravitación Universal matemática a $G = 1.0$. Por efecto cascada, todas las métricas de Masa, Longitud y Tiempo escalan armónicamente entre sí asumiendo condiciones locales (ideal para problemas teóricos computacionales).

### Parámetros de Simulación

- **Paso Temporal ($\Delta t$):** `0.01` por ciclo, logrando el equilibrio entre velocidad computacional y un control sano de los errores de discretización de Euler.
- **Parámetro de Suavizado ($\epsilon$):** `0.1`. Frena matemáticamente la fuerza gravitacional tendiente a infinito cuando las partículas experimentan distancias relativas intercósmicas casi nulas (evitando divisiones catastróficas por cero).
- **Criterio de Parada ($t_{fin}$):** El benchmark por defecto simula alrededor de `10.0` unidades de tiempo integradas a lo largo de un ciclo For finito preestablecido.

### Tolerancia Numérica (Delta de Validación de Coma Flotante)

Al comparar en las pruebas unitarias las rutinas seriales contra las rutinas paralelizadas con OpenMP, se evita estrictamente usar exactitud binaria (`==`).
Debido a que la aritmética de punto flotante en el estándar IEEE 754 **no es asociativa** (($a + b) + c \neq a + (b + c)$), las acumulaciones de reducciones numéricas concurrentes arrojan una suma final expuesta a cambios de paridad en el último bit según el orden del despachador de hilos. Por tal motivo, acordamos un **delta explícito de `1e-12` a través de Catch2 (`Approx().margin(1e-12)`)**. Esto previene falsos fallos en integraciones continuas, manteniendo al mismo tiempo un cerco férreo que sí es capaz de detectar roturas de _race conditions_ groseras.

---

## 4. Guía de Uso (Compilación y Ejecución)

### Instrucciones de Compilación

El proyecto utiliza GNU Make para simplificar eficientemente la compilación, especificamente con _g++ versión 15_.

```bash
# Limpiar el entorno de artefactos pre-existentes
make clean

# Compilar proyecto normal (crea el ejecutable central "nbody")
make all
```

### Ejecución de Pruebas Automáticas

La suite compilada estáticamente (Catch2) con todos los unit tests integrados se puede compilar y ejecutar con:

```bash
make test
```

Esto procesará en la terminal cada métrica de concurrencia, analógica estricta y regresión programada.

### Ejecución con Docker

La imagen de contenedor definida en [nbody_2d/Dockerfile](nbody_2d/Dockerfile) usa la base `nvidia/cuda:12.4.1-devel-ubuntu22.04`.
Sirve para compilar el proyecto y ejecutar `make test` en CPU sin usar GPU, pero si se quiere ejecutar el soporte CUDA del proyecto el host debe tener:

- GPU NVIDIA funcional.
- Driver NVIDIA compatible con CUDA 12.x.
- `nvidia-container-toolkit` configurado si se va a pasar la GPU al contenedor.

Para el smoke opcional de GPU se usa una imagen separada en [nbody_2d/Dockerfile.cuda](nbody_2d/Dockerfile.cuda), que permite verificar `nvcc` sin tocar el gate CPU principal.

### Benchmark GPU en preparación

El benchmark CPU sigue siendo el baseline actual. Al ejecutar `make benchmark` también se generan artefactos de planificación para la futura parte CUDA:

- `gpu_benchmark_plan.dat`
- `gpu_kernel_only.dat`
- `gpu_end_to_end.dat`
- `cpu_vs_gpu.dat`

Esos archivos dejan preparado el esquema de comparación pedido por el laboratorio, pero la implementación GPU real sigue pendiente.

### Reproducibilidad

Para reproducir exactamente nuestros laboratorios de rendimiento:

```bash
# Ejecuta e inicializa el modelo base emitiendo .dat a disco
make analysis

# Utiliza GNUPlot para plotear dinámicamente gráficos de resultados
make plot

# Dispara la ejecución del cronómetro estadístico (Escalamiento)
make benchmark
```

---

## 5. Justificaciones Adicionales

### Cobertura de Pruebas

Todas nuestras clases públicas (e.g. `Particle`, `NBodySystem`, `Integrator` y `MetricsCalculator`) se encuentran 100% testeadas activamente por suite.

- No obviamos los testeos intencionalmente destructivos: contamos con validaciones de "regresión catastrófica" aislando distancias relativas nulas intencionales sin suavizado para testear desviaciones numéricas.
- Confiamos explícitamente en validaciones de macros atómicas estables para el chequeo de "Race conditions", descartando fallos esporádicos inducidos sin previsualización de `Sanitizers`.

### Supuestos y Límites (Hardware/Modelo)

1. **Asimetría Computacional:** Al disponerse de un enfoque simple tipo _All-Pairs_ $O(N^2)$, el techo de desempeño de paralelismo lineal colapsa severamente ante miles de elementos sin integrarse paralelismos algorítmicos jerárquicos adyacentes tipo _Barnes-Hut_.
2. **Coherencia de Caché:** La optimización anti-false sharing para OpenMP presuponen tamaños de L1/L2 estáticos promedios para procesadores modernos, por ende el benchmark idealizado fue sujeto y atado intrínsecamente al hardware particular donde se efectuó.
