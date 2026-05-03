# Documentación Técnica — Rol 5: Calidad, CI y Visualización
## Proyecto N-Body 2D — Sistemas Distribuidos

---

## Tabla de contenidos

1. [Introducción y contexto](#1-introducción-y-contexto)
2. [Estructura de archivos creados](#2-estructura-de-archivos-creados)
3. [Framework de testing: Catch2 v3](#3-framework-de-testing-catch2-v3)
4. [Pruebas unitarias — test_Particle.cpp](#4-pruebas-unitarias--test_particlecpp)
5. [Pruebas de integración — test_NBodySystem.cpp](#5-pruebas-de-integración--test_nbodysystemcpp)
6. [Sistema de compilación — Makefile](#6-sistema-de-compilación--makefile)
7. [Contenedor Docker — Dockerfile](#7-contenedor-docker--dockerfile)
8. [Pipeline CI — GitHub Actions](#8-pipeline-ci--github-actions)
9. [Resultados de verificación](#9-resultados-de-verificación)
10. [Cómo usar todo](#10-cómo-usar-todo)

---

## 1. Introducción y contexto

El rol 5 en este laboratorio es responsable de la **infraestructura de calidad**: garantizar que el código que producen los demás roles funcione correctamente, sea reproducible en cualquier entorno, y que los errores se detecten automáticamente antes de que lleguen al repositorio principal.

Para eso se implementaron tres sistemas interconectados:

```
Desarrollador hace commit
        │
        ▼
GitHub Actions detecta el push
        │
        ├── Job 1: make test (Catch2)
        │     Compila el simulador + los tests
        │     Ejecuta 22 casos de prueba
        │     ¿Alguno falla? → pipeline rojo, PR bloqueado
        │
        └── Job 2: docker build
              Construye la imagen multi-stage
              (también ejecuta los tests adentro)
              ¿Falla? → pipeline rojo
```

Si todo pasa, el código queda validado en una máquina limpia y neutral (no en la máquina del desarrollador).

---

## 2. Estructura de archivos creados

### Archivos nuevos

```
lab-1-distri/
│
├── .github/
│   └── workflows/
│       └── ci.yml                  ← Pipeline de Integración Continua
│
└── nbody_2d/
    ├── Makefile                    ← Sistema de compilación (antes vacío)
    ├── Dockerfile                  ← Contenedor Docker (antes vacío)
    └── tests/
        ├── catch_amalgamated.hpp   ← Catch2 v3 — header  (14,570 líneas)
        ├── catch_amalgamated.cpp   ← Catch2 v3 — impl    (12,394 líneas)
        ├── test_main.cpp           ← Punto de entrada (referencia)
        ├── test_Particle.cpp       ← 8 pruebas unitarias
        └── test_NBodySystem.cpp    ← 14 pruebas de integración
```

### Archivos del proyecto (no modificados)

Estos archivos existían antes y **no se tocaron**. Los tests los usan tal como están:

| Archivo | Clase | Responsabilidad |
|---|---|---|
| `Particle.h / .cpp` | `Particle` | Masa, posición, velocidad, aceleración |
| `NBodySystem.h / .cpp` | `NBodySystem` | Vector de partículas, cómputo de fuerzas |
| `NBodySimulator.h / .cpp` | `NBodySimulator` | Integrador Euler, cálculo de energía |
| `main.cpp` | — | Punto de entrada del simulador |

---

## 3. Framework de testing: Catch2 v3

### ¿Qué es Catch2?

Catch2 es el framework de testing para C++ más popular en proyectos modernos. Permite escribir tests directamente en C++ sin infraestructura externa: se compilan junto con el código del proyecto y producen un ejecutable que reporta los resultados.

### Versión 3 — distribución amalgamada

La versión 3 de Catch2 abandonó el modelo de "un único header" de v2. Ahora se distribuye en dos archivos que van directamente en el repositorio:

| Archivo | Función | Tamaño |
|---|---|---|
| `catch_amalgamated.hpp` | Definiciones, macros, tipos de Catch2 | ~530 KB, 14,570 líneas |
| `catch_amalgamated.cpp` | Implementación completa + **genera el `main()`** | ~438 KB, 12,394 líneas |

Descarga (versión v3.14.0):
```bash
curl -L "https://raw.githubusercontent.com/catchorg/Catch2/v3.14.0/extras/catch_amalgamated.hpp" \
     -o tests/catch_amalgamated.hpp

curl -L "https://raw.githubusercontent.com/catchorg/Catch2/v3.14.0/extras/catch_amalgamated.cpp" \
     -o tests/catch_amalgamated.cpp
```

### Diferencia importante con Catch2 v2

| Aspecto | Catch2 v2 (legacy) | Catch2 v3 (actual) |
|---|---|---|
| Distribución | Un solo `.hpp` | `.hpp` + `.cpp` |
| `main()` del test runner | `#define CATCH_CONFIG_MAIN` en un archivo propio | Ya incluido en `catch_amalgamated.cpp` |
| Namespace de `Approx` | Global: `Approx(1.0)` | Explícito: `Catch::Approx(1.0)` |

### Anatomía de un test

```cpp
// 1. Incluir el header de Catch2 (siempre en todos los archivos de test)
#include "catch_amalgamated.hpp"

// 2. Traer Approx al scope actual (necesario en v3)
using Catch::Approx;

// 3. Definir un caso de prueba con nombre y etiquetas
TEST_CASE("Descripción clara de qué se prueba", "[ClaseAfectada][categoria]") {

    // Setup: construir el objeto en estado conocido
    Particle p(1.0, 0.0, 0.0);

    // Ejercitar: llamar al método que queremos probar
    p.setAcceleration(2.0, -3.0);
    p.kick(0.5);

    // Verificar: comprobar que el resultado es el esperado
    REQUIRE(p.getVX() == Approx(1.0));   // falla → test se detiene
    REQUIRE(p.getVY() == Approx(-1.5));
}
```

### Macros de verificación

| Macro | Comportamiento al fallar |
|---|---|
| `REQUIRE(expr)` | Detiene el test inmediatamente |
| `CHECK(expr)` | Continúa, reporta todos los fallos del test |
| `REQUIRE_THROWS(expr)` | Verifica que se lanza una excepción |

### `Approx` — comparación de punto flotante

Los números de punto flotante (`double`, `float`) nunca deben compararse con `==` directo porque las operaciones aritméticas acumulan errores de redondeo. `Approx` aplica una tolerancia relativa de `1e-7`:

```cpp
// MAL — puede fallar aunque el resultado sea "correcto"
REQUIRE(p.getVX() == 1.0);

// BIEN — tolerancia automática para punto flotante
REQUIRE(p.getVX() == Approx(1.0));
```

---

## 4. Pruebas unitarias — `test_Particle.cpp`

### ¿Qué es una prueba unitaria?

Una prueba unitaria verifica **una sola clase en completo aislamiento**:
- No usa objetos de otras clases
- Crea sus instancias desde cero al inicio de cada test
- Si falla, el problema está garantizadamente en esa clase

En este proyecto, `Particle` es la clase base de todo. Si `kick()` o `drift()` calculan mal, el integrador Euler entero es incorrecto. Por eso se testea exhaustivamente.

### Los 8 tests implementados

---

#### Test 1 — Constructor 

```cpp
TEST_CASE("Particle: el constructor inicializa todos los campos correctamente",
          "[Particle][constructor]") {
    Particle p(5.0, 1.0, 2.0);

    REQUIRE(p.getMass() == Approx(5.0));
    REQUIRE(p.getX()    == Approx(1.0));
    REQUIRE(p.getY()    == Approx(2.0));
    REQUIRE(p.getVX()   == Approx(0.0));  // velocidad inicial = 0
    REQUIRE(p.getVY()   == Approx(0.0));
    REQUIRE(p.getAX()   == Approx(0.0));  // aceleración inicial = 0
    REQUIRE(p.getAY()   == Approx(0.0));
}
```

**¿Por qué?** Si el constructor no pusiera velocidad y aceleración en cero, la primera iteración del simulador usaría basura de memoria como estado inicial. Este test garantiza el contrato del constructor.

---

#### Tests 2 y 3 — `kick()` (v = v + a·dt)

```cpp
TEST_CASE("Particle: kick() actualiza la velocidad con v = v + a*dt", "[Particle][kick]") {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(2.0, -3.0);
    p.kick(0.5);

    REQUIRE(p.getVX() == Approx(1.0));   // 0 + 2.0 * 0.5 = 1.0
    REQUIRE(p.getVY() == Approx(-1.5));  // 0 + (-3.0) * 0.5 = -1.5
}

TEST_CASE("Particle: kick() con dt=0 no cambia la velocidad", "[Particle][kick]") {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(10.0, 10.0);
    p.kick(0.0);

    REQUIRE(p.getVX() == Approx(0.0));
    REQUIRE(p.getVY() == Approx(0.0));
}
```

**¿Por qué dos tests para kick?** El test normal verifica la fórmula. El test con `dt=0` verifica el caso borde: si el simulador se llama sin avance de tiempo, la velocidad no debe cambiar. Es un caso que puede ocurrir en configuraciones de prueba.

---

#### Tests 4 y 5 — `drift()` (r = r + v·dt)

```cpp
TEST_CASE("Particle: drift() actualiza la posición con r = r + v*dt", "[Particle][drift]") {
    Particle p(1.0, 0.0, 0.0);
    p.setVelocity(4.0, -2.0);
    p.drift(0.25);

    REQUIRE(p.getX() == Approx(1.0));   // 0 + 4.0 * 0.25 = 1.0
    REQUIRE(p.getY() == Approx(-0.5));  // 0 + (-2.0) * 0.25 = -0.5
}

TEST_CASE("Particle: drift() con dt=0 no cambia la posición", "[Particle][drift]") {
    Particle p(1.0, 3.0, 7.0);
    p.setVelocity(100.0, 100.0);
    p.drift(0.0);

    REQUIRE(p.getX() == Approx(3.0));
    REQUIRE(p.getY() == Approx(7.0));
}
```

**Relación con el integrador:** El integrador Euler en `NBodySimulator::integrateEuler()` llama exactamente a `kick(dt)` seguido de `drift(dt)`. Si drift no calcula bien, las partículas no se moverían correctamente en la simulación.

---

#### Tests 6 y 7 — Setters (`setAcceleration`, `addAcceleration`)

```cpp
TEST_CASE("Particle: setAcceleration() asigna valores correctamente", "[Particle][setters]") {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(3.5, -1.2);

    REQUIRE(p.getAX() == Approx(3.5));
    REQUIRE(p.getAY() == Approx(-1.2));
}

TEST_CASE("Particle: addAcceleration() acumula sobre la aceleración existente", "[Particle][setters]") {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(1.0, 2.0);
    p.addAcceleration(0.5, -1.0);

    REQUIRE(p.getAX() == Approx(1.5));  // 1.0 + 0.5
    REQUIRE(p.getAY() == Approx(1.0));  // 2.0 + (-1.0)
}
```

**¿Por qué `addAcceleration` importa?** En implementaciones paralelas del simulador, cada hilo calcula la contribución de un subconjunto de partículas y usa `addAcceleration` para acumular. Si la acumulación fuera incorrecta, las fuerzas calculadas en paralelo estarían mal.

---

#### Test 8 — Secuencia completa: kick + drift (un paso Euler)

```cpp
TEST_CASE("Particle: secuencia kick + drift produce movimiento correcto", "[Particle][integration]") {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(0.0, 9.8);  // "gravedad"

    p.kick(1.0);   // vy = 0 + 9.8 * 1.0 = 9.8
    p.drift(1.0);  // y  = 0 + 9.8 * 1.0 = 9.8

    REQUIRE(p.getVY() == Approx(9.8));
    REQUIRE(p.getY()  == Approx(9.8));
}
```

Este test simula manualmente lo que hace `integrateEuler()` y verifica que la secuencia produce el resultado físicamente correcto: una partícula con aceleración gravitacional hacia abajo se mueve hacia abajo.

---

## 5. Pruebas de integración — `test_NBodySystem.cpp`

### ¿Qué es una prueba de integración?

Una prueba de integración verifica cómo **varias clases colaboran entre sí**. El objetivo no es saber si `Particle` funciona (eso ya lo prueban las pruebas unitarias), sino si `NBodySystem` y `NBodySimulator` las usan correctamente juntas.

### Los 14 tests implementados

---

#### Tests 1 y 2 — Inicialización de NBodySystem

```cpp
TEST_CASE("NBodySystem: el constructor almacena G y epsilon correctamente") {
    NBodySystem sys(6.674e-11, 0.05);

    REQUIRE(sys.getG_const() == Approx(6.674e-11));
    REQUIRE(sys.getEps()     == Approx(0.05));
    REQUIRE(sys.getCount()   == 0);
}

TEST_CASE("NBodySystem: sistema vacío tiene cero partículas") {
    NBodySystem sys(1.0, 0.05);
    REQUIRE(sys.getBodies().empty());
}
```

`G` y `epsilon` son parámetros críticos de la simulación. `G` escala la fuerza gravitacional; `epsilon` es el factor de suavizado que evita singularidades cuando dos partículas se acercan mucho. Si no se almacenan correctamente, toda la física del sistema es incorrecta.

---

#### Tests 3 y 4 — `addParticle`

```cpp
TEST_CASE("NBodySystem: addParticle incrementa el contador") {
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 0.0, 0.0);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 1);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 2);
}

TEST_CASE("NBodySystem: la partícula añadida conserva sus propiedades") {
    NBodySystem sys(1.0, 0.05);
    Particle p(7.5, 3.0, -4.0);
    sys.addParticle(p);

    REQUIRE(sys.getBodies()[0].getMass() == Approx(7.5));
    REQUIRE(sys.getBodies()[0].getX()    == Approx(3.0));
    REQUIRE(sys.getBodies()[0].getY()    == Approx(-4.0));
}
```

El segundo test es particularmente importante: verifica que `addParticle` hace una **copia correcta** de la partícula. En C++, si `push_back` copiara por referencia en lugar de por valor, modificar la `Particle` original cambiaría el sistema, lo cual sería un bug difícil de detectar.

---

#### Tests 5, 6, 7 y 8 — Generadores de escenarios

```cpp
TEST_CASE("NBodySystem: randomSystem genera exactamente N partículas") {
    NBodySystem sys(1.0, 0.05);
    sys.randomSystem(50, 42);
    REQUIRE(sys.getCount() == 50);
}

TEST_CASE("NBodySystem: randomSystem con distinta semilla → posiciones distintas") {
    NBodySystem sys1(1.0, 0.05), sys2(1.0, 0.05);
    sys1.randomSystem(10, 1);
    sys2.randomSystem(10, 2);

    bool diferente = (sys1.getBodies()[0].getX() != sys2.getBodies()[0].getX()) ||
                     (sys1.getBodies()[0].getY() != sys2.getBodies()[0].getY());
    REQUIRE(diferente);
}

TEST_CASE("NBodySystem: bynarySystem genera exactamente 3 partículas") {
    NBodySystem sys(1.0, 0.05);
    sys.bynarySystem(99);
    REQUIRE(sys.getCount() == 3);
}

TEST_CASE("NBodySystem: diskSystem genera exactamente N partículas") {
    NBodySystem sys(1.0, 0.05);
    sys.diskSystem(30, 7);
    REQUIRE(sys.getCount() == 30);
}
```

El test de semillas distintas es importante porque verifica que el generador usa la semilla correctamente: si dos semillas distintas produjeran el mismo sistema, `randomSystem` no sería aleatorio.

---

#### Tests 9 y 10 — Física gravitacional

Este es el núcleo de la simulación. Se verifica que `computeAccelerations()` implementa correctamente la ley de gravitación universal de Newton.

```
         F = G * m1 * m2 / r²
         
         p0 (masa=1)              p1 (masa=1)
         en x=0                   en x=10
         ●──────────────────────────●
         
         ← F₁₀ (p1 sobre p0)    F₀₁ (p0 sobre p1) →
         
         p0 debe acelerarse en +X
         p1 debe acelerarse en -X
         |ax_p0| == |ax_p1| (mismas masas → 3ª Ley de Newton)
```

```cpp
TEST_CASE("computeAccelerations produce atracción gravitacional correcta") {
    NBodySystem sys(1.0, 0.0);  // G=1, eps=0

    Particle p0(1.0,  0.0, 0.0);
    Particle p1(1.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);
    sys.computeAccelerations();

    const auto& bodies = sys.getBodies();

    REQUIRE(bodies[0].getAX() > 0.0);   // p0 → +X (hacia p1)
    REQUIRE(bodies[1].getAX() < 0.0);   // p1 → -X (hacia p0)
    REQUIRE(bodies[0].getAY() == Approx(0.0));  // sin componente Y
    REQUIRE(bodies[1].getAY() == Approx(0.0));
    // 3ª Ley de Newton: masas iguales → magnitudes iguales
    REQUIRE(std::abs(bodies[0].getAX()) == Approx(std::abs(bodies[1].getAX())));
}

TEST_CASE("Cuerpo masivo produce mayor aceleración en cuerpo ligero") {
    // a = G*M/r² → p0 (masa=1) siente G*100/100=1.0, p1 (masa=100) siente G*1/100=0.01
    NBodySystem sys(1.0, 0.0);
    Particle p0(  1.0,  0.0, 0.0);
    Particle p1(100.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);
    sys.computeAccelerations();

    REQUIRE(std::abs(bodies[0].getAX()) > std::abs(bodies[1].getAX()));
}
```

**¿Por qué eps=0 en estos tests?** Con `eps=0` la fórmula `rSquared = dx² + dy² + eps²` se reduce a la fuerza gravitacional pura `r²`, lo que hace los cálculos manuales exactos y verificables sin ambigüedad.

---

#### Tests 11, 12 y 13 — NBodySimulator + integrador Euler

```cpp
TEST_CASE("Partícula aislada sin fuerzas no se mueve") {
    // Un solo cuerpo → no hay pares → aceleración = 0 → no debe moverse
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 5.0, 7.0);
    sys.addParticle(p);

    NBodySimulator sim(&sys, 0.01);
    sys.computeAccelerations();  // ax=ay=0 (no hay otros cuerpos)
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() == Approx(5.0));
    REQUIRE(sys.getParticles()[0].getY() == Approx(7.0));
}

TEST_CASE("Partícula con velocidad inicial se desplaza correctamente") {
    // vx=1.0, a=0, dt=0.1 → x = 0 + 1.0*0.1 = 0.1
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 0.0, 0.0);
    sys.addParticle(p);
    sys.getParticles()[0].setVelocity(1.0, 0.0);

    NBodySimulator sim(&sys, 0.1);
    sys.computeAccelerations();
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() == Approx(0.1));
    REQUIRE(sys.getParticles()[0].getY() == Approx(0.0));
}

TEST_CASE("Dos partículas se acercan tras un paso de integración") {
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(1.0, 10.0, 0.0));

    NBodySimulator sim(&sys, 0.01);
    sys.computeAccelerations();
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() > 0.0);   // p0 se acercó a p1
    REQUIRE(sys.getParticles()[1].getX() < 10.0);  // p1 se acercó a p0
}
```

El último test verifica el ciclo completo: `computeAccelerations()` → `integrateEuler()` → las partículas se movieron en la dirección físicamente correcta. Es el test de mayor nivel de integración: valida que las tres clases (`Particle`, `NBodySystem`, `NBodySimulator`) funcionan juntas.

---

## 6. Sistema de compilación — Makefile

### El problema de macOS

En macOS, `g++` es en realidad un alias de `clang++` (Apple Clang). A diferencia de Linux, el compilador en macOS no siempre localiza automáticamente los headers de la biblioteca estándar de C++ (`<iostream>`, `<vector>`, `<cmath>`, etc.). Necesita dos flags adicionales:

| Flag | Función |
|---|---|
| `-stdlib=libc++` | Usar libc++ (la stdlib de LLVM/Apple) en lugar de libstdc++ |
| `-isysroot $(xcrun --show-sdk-path)` | Apuntar al SDK de Xcode donde están los headers del sistema |

### Detección automática con `uname`

El Makefile detecta el sistema operativo en tiempo de ejecución:

```makefile
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
    CXXFLAGS += -stdlib=libc++ -isysroot $(shell xcrun --show-sdk-path)
endif
```

- `$(shell uname)` ejecuta el comando `uname` y captura su salida
- En macOS devuelve `Darwin`; en Linux devuelve `Linux`
- Con `ifeq` se agregan los flags **solo si es macOS**
- En Linux (donde corre el CI de GitHub Actions) estos flags no se agregan y todo funciona directamente

### Los tres targets explicados

#### `make all` (target por defecto)

```makefile
nbody: main.cpp $(SRCS)
    $(CXX) $(CXXFLAGS) $^ -o $@
```

Compila `main.cpp + Particle.cpp + NBodySystem.cpp + NBodySimulator.cpp` en el ejecutable `nbody`. La variable automática `$^` expande todos los prerrequisitos y `$@` es el nombre del target.

#### `make test`

```makefile
TEST_SRCS = tests/catch_amalgamated.cpp \
            tests/test_Particle.cpp \
            tests/test_NBodySystem.cpp

test: $(SRCS) $(TEST_SRCS)
    $(CXX) $(CXXFLAGS) $^ -o test_runner
    ./test_runner
```

**¿Por qué no incluye `main.cpp`?** Porque `catch_amalgamated.cpp` ya provee su propio `main()`. Si se incluyera `main.cpp` también, el linker fallaría con error de símbolo duplicado:
```
error: duplicate symbol 'main'
```

**Flujo:**
1. Compila `Particle.cpp + NBodySystem.cpp + NBodySimulator.cpp` (el simulador) junto con `catch_amalgamated.cpp + test_Particle.cpp + test_NBodySystem.cpp` (el framework + los tests)
2. Produce el ejecutable `test_runner`
3. Lo ejecuta: Catch2 descubre todos los `TEST_CASE` del ejecutable y los corre
4. Si alguno falla → exit code `!= 0` → `make` falla → CI falla

#### `make clean`

```makefile
clean:
    rm -f nbody test_runner *.dat
```

Elimina los binarios compilados y los archivos `.dat` de datos que genera el simulador (`trajectories.dat`, `energy_timeseries.dat`). El patrón `*.dat` es importante para no dejar archivos de simulaciones anteriores que puedan confundir a tests o al Docker.

---

## 7. Contenedor Docker — Dockerfile

### Arquitectura multi-stage

```dockerfile
FROM gcc:13 AS builder

WORKDIR /app
COPY . .

RUN make all    # compilar el simulador
RUN make test   # compilar Y ejecutar los tests

FROM debian:bookworm-slim AS runtime

WORKDIR /app
COPY --from=builder /app/nbody .

VOLUME ["/app"]
CMD ["./nbody"]
```

### ¿Por qué dos stages?

Un Dockerfile de un solo stage que contuviera el compilador y los fuentes terminaría pesando ~1.4 GB porque `gcc:13` es una imagen grande. Con multi-stage:

| Stage | Image base | Qué contiene | Tamaño aprox. |
|---|---|---|---|
| `builder` | `gcc:13` | Compilador, headers, fuentes, binarios | ~1.4 GB |
| `runtime` | `debian:bookworm-slim` | Solo el binario `./nbody` | ~80 MB |

La imagen final nunca incluye el compilador ni el código fuente.

### ¿Por qué `RUN make test` dentro del Dockerfile?

Cuando `docker build` ejecuta un `RUN` y ese comando retorna un exit code distinto de 0, **el build entero falla**. Esto significa:

```
make test   →  algún test falla
             →  exit code != 0
             →  docker build falla
             →  la imagen NO se crea
             →  Job 2 del CI falla
```

Esto garantiza que **no puede existir una imagen Docker con código roto**. La imagen solo se construye si todos los tests pasan.

### Uso del volumen

El simulador genera dos archivos de datos:
- `trajectories.dat` — posiciones y velocidades en cada paso de tiempo
- `energy_timeseries.dat` — energía cinética, potencial y total por paso

Dentro del contenedor estos archivos se crean en `/app`. Para acceder a ellos desde el host:

```bash
# El directorio ./output en el host se monta en /app en el contenedor
docker run -v $(pwd)/output:/app nbody_lab1
```

---

## 8. Pipeline CI — GitHub Actions

### ¿Qué es la Integración Continua?

La Integración Continua (CI) es la práctica de compilar y testear el código automáticamente en una máquina neutral (no en la del desarrollador) cada vez que se hace un commit. El objetivo es detectar problemas inmediatamente y evitar el clásico "en mi máquina sí funciona".

### Trigger: cuándo se activa

```yaml
on:
  push:
    branches: [main, master]
  pull_request:
    branches: [main, master]
```

- **`push`**: cada vez que se hace `git push` a `main` o `master`
- **`pull_request`**: cada vez que se abre o actualiza un PR hacia esas ramas. Si el pipeline falla, GitHub muestra el PR en rojo y bloquea el merge

### Job 1: `build-and-test`

```yaml
build-and-test:
  name: Build & Test (Catch2)
  runs-on: ubuntu-latest

  steps:
    - name: Checkout del repositorio
      uses: actions/checkout@v4

    - name: Instalar g++ y make
      run: |
        sudo apt-get update -qq
        sudo apt-get install -y g++ make

    - name: Compilar y ejecutar tests (make test)
      working-directory: nbody_2d
      run: make test

    - name: Compilar binario principal (make all)
      working-directory: nbody_2d
      run: make all
```

**¿Por qué `actions/checkout@v4`?** La máquina virtual de GitHub empieza sin el código del repositorio. `checkout` es la acción oficial de GitHub que clona el repositorio en la máquina del runner.

**¿Por qué instalar g++ si ya usamos Docker?** Los dos jobs son independientes. El Job 1 valida el código directamente en el sistema operativo (sin Docker) para dar feedback rápido. El Job 2 valida que el Docker también funciona. Si solo usáramos Docker, tardaríamos más en detectar errores.

**`working-directory: nbody_2d`**: El Makefile está en `nbody_2d/`, no en la raíz del repo. Este parámetro equivale a `cd nbody_2d && make test`.

**En Linux, el Makefile no agrega `-stdlib=libc++` ni `-isysroot`** (recordar la detección con `uname`), por lo que `g++` en Ubuntu funciona directamente con los headers del sistema.

### Job 2: `docker-build`

```yaml
docker-build:
  name: Docker Build
  runs-on: ubuntu-latest
  needs: build-and-test   # ← dependencia explícita

  steps:
    - uses: actions/checkout@v4

    - name: Construir imagen Docker
      working-directory: nbody_2d
      run: docker build -t nbody_lab1 .
```

**`needs: build-and-test`**: este job solo corre si el Job 1 pasó. Si los tests fallan, no tiene sentido construir el Docker. Además, si los tests pasan en Job 1 pero el Dockerfile está roto, el Job 2 lo detectará.

### Flujo completo en GitHub

```
git push origin main
      │
      ▼
GitHub Actions detecta el push
      │
      ├──► Job 1: build-and-test (ubuntu-latest)
      │    │  1. Clona el repo
      │    │  2. apt-get install g++ make
      │    │  3. make test → ¿pasan los 22 tests?
      │    │  4. make all  → ¿compila el binario?
      │    └── ✅ PASS o ❌ FAIL
      │
      └──► Job 2: docker-build (ubuntu-latest) [espera Job 1]
           │  1. Clona el repo
           │  2. docker build → dentro: make test + make all
           └── ✅ PASS o ❌ FAIL
```

---

## 9. Resultados de verificación

### `make test` en macOS

```bash
g++ -std=c++17 -O2 -Wall -Wextra -stdlib=libc++ \
    -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX.sdk \
    Particle.cpp NBodySystem.cpp NBodySimulator.cpp \
    tests/catch_amalgamated.cpp tests/test_Particle.cpp tests/test_NBodySystem.cpp \
    -o test_runner

./test_runner
Randomness seeded to: 2594703778
===============================================================================
All tests passed (51 assertions in 22 test cases)
```

| Métrica | Valor |
|---|---|
| Test cases | 22 |
| Assertions | 51 |
| Resultado | ✅ All tests passed |
| Warnings | 4 (variables no usadas en NBodySimulator.cpp — código preexistente) |

### Warnings detectados

Durante la compilación con `-Wall -Wextra` se detectaron 4 warnings en `NBodySimulator.cpp`:

```
NBodySimulator.cpp:15: warning: unused variable 'vx'
NBodySimulator.cpp:16: warning: unused variable 'vy'
NBodySimulator.cpp:17: warning: unused variable 'ax'
NBodySimulator.cpp:18: warning: unused variable 'ay'
```

Estas variables son declaradas pero no usadas en `integrateEuler()`. No afectan al funcionamiento, pero son código muerto. Son de código preexistente (no del Rol 5).

---

## 10. Cómo usar todo

### Compilar y ejecutar el simulador

```bash
cd nbody_2d

# Compilar
make

# Ejecutar
./nbody
```

### Correr los tests

```bash
cd nbody_2d
make test
```

Salida esperada:
```
Randomness seeded to: XXXXXXXXXX
===============================================================================
All tests passed (51 assertions in 22 test cases)
```

### Limpiar artefactos

```bash
make clean
# Elimina: nbody, test_runner, *.dat
```

### Docker

```bash
cd nbody_2d

# Construir la imagen (también ejecuta los tests)
docker build -t nbody_lab1 .

# Correr el simulador en el contenedor
docker run nbody_lab1

# Conservar los archivos .dat generados
mkdir -p output
docker run -v $(pwd)/output:/app nbody_lab1
```

### CI en GitHub

El pipeline CI se activa automáticamente. Para verlo:
1. Ir al repositorio en GitHub
2. Pestaña **Actions**
3. Ver el pipeline `CI — N-Body 2D`

---

## 11. Métricas y Benchmarks (Rol 2)

Se han incorporado dos nuevas herramientas al entorno para evaluar el estado físico del sistema y medir el rendimiento algorítmico del integrador implementado, fuertemente enfocados en la escalabilidad paralela usando **OpenMP**:

### Compilación y Ejecución del Benchmark

Para ejecutar el módulo de evaluación de métricas físicas y su respectivo análisis de escalabilidad de hilos:

```bash
cd nbody_2d

# Compilar el binario de benchmark
make benchmark

# Ejecutar el perfilador y exportar datos
./benchmark
```

### ¿Qué se está calculando internamente?

1. **`MetricsCalculator`**: Demuestra el uso de sincronización avanzada en OpenMP (`atomic`, `reduction`, `firstprivate`, `lastprivate`). Calcula y exporta a `energy_timeseries.dat`:
   - Energía Cinética y Potencial (con factor de suavizado $\epsilon$).
   - Centro de Masas ($R_{cm}$) y Radio Cuadrático Medio ($R_{rms}$).
   - Momento Lineal ($P$) y Distancia Mínima.

2. **`Benchmark`**: Utiliza `omp_get_wtime()` iterando de 1 hasta 4 hilos (configurable), repitiendo cada escenario múltiples veces para obtener un T promedio y su desviación estándar ($\sigma_T$).
   - Exporta resultados a `scaling_analysis.dat`.
   - Propaga el error estadístico para el Speedup ($S_p$) y la Eficiencia ($E_p$).
   - Estima matemáticamente la fracción serial del algoritmo según la *Ley de Amdahl*.

---

*Documentación técnica del repositorio de N-Body 2D.*
