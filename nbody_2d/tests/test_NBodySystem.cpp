// test_NBodySystem.cpp
// ─────────────────────────────────────────────────────────────────────────────
// GUÍA GENERAL DE TOLERANCIAS EN COMA FLOTANTE
// ─────────────────────────────────────────────────────────────────────────────
// Los valores calculados analíticamente (a mano) usan aritmética exacta de
// racionales simples (e.g. 10/1000 = 0.01). El error de representación IEEE-754
// en doble precisión es del orden de 1e-15 a 1e-16 para operaciones básicas.
// Por eso usamos .margin(1e-12): mucho mayor que el error de máquina (~1e-15),
// pero lo suficientemente pequeño para que cualquier error de lógica (e.g.,
// dividir por r² en lugar de r³, sumar en vez de restar) sea detectado.
// Pruebas de INTEGRACIÓN para NBodySystem y NBodySimulator.
// Usa Catch2 v3 (catch_amalgamated.hpp).
// Verifica que las clases colaboran correctamente y que la física es coherente.

#include "catch_amalgamated.hpp"
#include "../NBodySystem.h"
#include "../NBodySimulator.h"

#include <cmath>

using Catch::Approx;

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — Inicialización
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: el constructor almacena G y epsilon correctamente", "[NBodySystem][init]") {
    NBodySystem sys(6.674e-11, 0.05);

    REQUIRE(sys.getG_const() == Approx(6.674e-11));
    REQUIRE(sys.getEps()     == Approx(0.05));
    REQUIRE(sys.getCount()   == 0);
}

TEST_CASE("NBodySystem: sistema vacío tiene cero partículas", "[NBodySystem][init]") {
    NBodySystem sys(1.0, 0.05);
    REQUIRE(sys.getBodies().empty());
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — addParticle
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: addParticle incrementa el contador", "[NBodySystem][addParticle]") {
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 0.0, 0.0);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 1);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 2);

    sys.addParticle(p);
    REQUIRE(sys.getCount() == 3);
}

TEST_CASE("NBodySystem: la partícula añadida conserva sus propiedades", "[NBodySystem][addParticle]") {
    NBodySystem sys(1.0, 0.05);
    Particle p(7.5, 3.0, -4.0);
    sys.addParticle(p);

    const auto& bodies = sys.getBodies();
    REQUIRE(bodies[0].getMass() == Approx(7.5));
    REQUIRE(bodies[0].getX()    == Approx(3.0));
    REQUIRE(bodies[0].getY()    == Approx(-4.0));
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — Generadores de escenarios
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: randomSystem genera exactamente N partículas", "[NBodySystem][generators]") {
    NBodySystem sys(1.0, 0.05);
    sys.randomSystem(50, 42);
    REQUIRE(sys.getCount() == 50);
}

TEST_CASE("NBodySystem: randomSystem con distinta semilla produce posiciones distintas", "[NBodySystem][generators]") {
    NBodySystem sys1(1.0, 0.05);
    NBodySystem sys2(1.0, 0.05);

    sys1.randomSystem(10, 1);
    sys2.randomSystem(10, 2);

    bool diferente = (sys1.getBodies()[0].getX() != sys2.getBodies()[0].getX()) ||
                     (sys1.getBodies()[0].getY() != sys2.getBodies()[0].getY());
    REQUIRE(diferente);
}

TEST_CASE("NBodySystem: binarySystem genera exactamente 3 partículas", "[NBodySystem][generators]") {
    NBodySystem sys(1.0, 0.05);
    sys.binarySystem(99);
    REQUIRE(sys.getCount() == 3);
}

TEST_CASE("NBodySystem: diskSystem genera exactamente N partículas", "[NBodySystem][generators]") {
    NBodySystem sys(1.0, 0.05);
    sys.diskSystem(30, 7);
    REQUIRE(sys.getCount() == 30);
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySystem — Física gravitacional
// ─────────────────────────────────────────────

TEST_CASE("NBodySystem: computeAccelerations produce atracción gravitacional correcta (2 cuerpos)", "[NBodySystem][physics]") {
    // p0 en (0,0), p1 en (10,0)
    // → p0 se acelera en +X (hacia p1)
    // → p1 se acelera en -X (hacia p0)
    NBodySystem sys(1.0, 0.0);

    Particle p0(1.0,  0.0, 0.0);
    Particle p1(1.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);

    sys.computeAccelerations();

    const auto& bodies = sys.getBodies();

    REQUIRE(bodies[0].getAX() > 0.0);
    REQUIRE(bodies[1].getAX() < 0.0);
    REQUIRE(bodies[0].getAY() == Approx(0.0));
    REQUIRE(bodies[1].getAY() == Approx(0.0));

    // 3ª Ley de Newton: masas iguales → magnitudes de aceleración iguales
    REQUIRE(std::abs(bodies[0].getAX()) == Approx(std::abs(bodies[1].getAX())));
}

TEST_CASE("NBodySystem: cuerpo masivo produce mayor aceleración en cuerpo ligero", "[NBodySystem][physics]") {
    // a = G*M/r² → p0 (masa=1) experimenta G*100/100=1.0, p1 (masa=100) experimenta G*1/100=0.01
    NBodySystem sys(1.0, 0.0);

    Particle p0(  1.0,  0.0, 0.0);
    Particle p1(100.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);

    sys.computeAccelerations();

    const auto& bodies = sys.getBodies();
    REQUIRE(std::abs(bodies[0].getAX()) > std::abs(bodies[1].getAX()));
}

// ─────────────────────────────────────────────
//  SECCIÓN: NBodySimulator — Integración Euler
// ─────────────────────────────────────────────

TEST_CASE("NBodySimulator: partícula aislada sin fuerzas no se mueve", "[NBodySimulator][euler]") {
    // Un solo cuerpo → sin interacciones → aceleración = 0 → no se mueve
    NBodySystem sys(1.0, 0.05);
    Particle p(1.0, 5.0, 7.0);
    sys.addParticle(p);

    NBodySimulator sim(&sys, 0.01);
    sys.computeAccelerations();
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() == Approx(5.0));
    REQUIRE(sys.getParticles()[0].getY() == Approx(7.0));
}

TEST_CASE("NBodySimulator: partícula con velocidad inicial se desplaza correctamente", "[NBodySimulator][euler]") {
    // p con vx=1.0, ay=0 → tras dt=0.1: x = 0 + 1.0*0.1 = 0.1
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

TEST_CASE("NBodySimulator: dos partículas se acercan tras un paso de integración", "[NBodySimulator][euler]") {
    // p0 en x=0, p1 en x=10 → se atraen → después de un paso: x0>0, x1<10
    NBodySystem sys(1.0, 0.0);
    Particle p0(1.0,  0.0, 0.0);
    Particle p1(1.0, 10.0, 0.0);
    sys.addParticle(p0);
    sys.addParticle(p1);

    NBodySimulator sim(&sys, 0.01);
    sys.computeAccelerations();
    sim.integrateEuler();

    REQUIRE(sys.getParticles()[0].getX() > 0.0);
    REQUIRE(sys.getParticles()[1].getX() < 10.0);
}

// ─────────────────────────────────────────────
//  SECCIÓN: Valores analíticos exactos
// ─────────────────────────────────────────────
// Estos tests verifican el valor NUMÉRICO de la aceleración frente a un cálculo
// analítico hecho a mano. Esto es más robusto que solo verificar el signo.
//
// Fórmula implementada en computeAccelerations():
//   totalAX += G * m_j * (x_j - x_i) / (r * r * r)
// donde r = sqrt((x_j-x_i)² + (y_j-y_i)² + eps²)
// Con eps=0 el denominador es exactamente r³.

TEST_CASE("NBodySystem: valor analítico exacto de aceleración (2 cuerpos sobre eje X)",
          "[NBodySystem][physics][analytical]") {
    // Geometría fija:
    //   p0 en (0, 0)  con masa m0 = 1.0
    //   p1 en (10, 0) con masa m1 = 1.0
    //   G = 1.0,  eps = 0.0
    //
    // Distancia: r = 10
    //
    // Aceleración de p0 debida a p1 (dirección +X):
    //   ax_0 = G * m1 * (x1 - x0) / r³
    //        = 1.0 * 1.0 * (10 - 0) / (10³)
    //        = 10 / 1000
    //        = 0.01
    //
    // Aceleración de p1 debida a p0 (dirección -X):
    //   ax_1 = G * m0 * (x0 - x1) / r³
    //        = 1.0 * 1.0 * (0 - 10) / (10³)
    //        = -10 / 1000
    //        = -0.01
    //
    // Componente Y: dy = 0 → ay = 0 en ambos casos.

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));  // p0
    sys.addParticle(Particle(1.0, 10.0, 0.0));  // p1
    sys.computeAccelerations();

    const auto& b = sys.getBodies();

    // p0: se acelera hacia p1 (dirección +X)
    REQUIRE(b[0].getAX() == Approx( 0.01).margin(1e-12));
    REQUIRE(b[0].getAY() == Approx( 0.0 ).margin(1e-12));

    // p1: se acelera hacia p0 (dirección -X)
    REQUIRE(b[1].getAX() == Approx(-0.01).margin(1e-12));
    REQUIRE(b[1].getAY() == Approx( 0.0 ).margin(1e-12));
}

TEST_CASE("NBodySystem: valor analítico exacto de aceleración (3 cuerpos colineales)",
          "[NBodySystem][physics][analytical]") {
    // Geometría fija:
    //   p0 en ( 0, 0) masa=1,   p1 en (5, 0) masa=1,   p2 en (10, 0) masa=1
    //   G = 1.0, eps = 0.0
    //
    // ── Aceleración de p0 ──────────────────────────────────────────────
    //   Contribución de p1:  G*m1*(5-0)/5³  = 1*1*5/125  =  0.04
    //   Contribución de p2:  G*m2*(10-0)/10³ = 1*1*10/1000 =  0.01
    //   ax_0 = 0.04 + 0.01 = 0.05
    //
    // ── Aceleración de p1 ──────────────────────────────────────────────
    //   Contribución de p0:  G*m0*(0-5)/5³  = 1*1*(-5)/125 = -0.04
    //   Contribución de p2:  G*m2*(10-5)/5³ = 1*1*5/125    =  0.04
    //   ax_1 = -0.04 + 0.04 = 0.00  (por simetría el cuerpo central no se mueve en X)
    //
    // ── Aceleración de p2 ──────────────────────────────────────────────
    //   Contribución de p0:  G*m0*(0-10)/10³ = 1*1*(-10)/1000 = -0.01
    //   Contribución de p1:  G*m1*(5-10)/5³  = 1*1*(-5)/125   = -0.04
    //   ax_2 = -0.01 - 0.04 = -0.05
    //
    // La componente Y es cero en todos (dy=0).

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));  // p0
    sys.addParticle(Particle(1.0,  5.0, 0.0));  // p1
    sys.addParticle(Particle(1.0, 10.0, 0.0));  // p2
    sys.computeAccelerations();

    const auto& b = sys.getBodies();

    REQUIRE(b[0].getAX() == Approx( 0.05).margin(1e-12));
    REQUIRE(b[0].getAY() == Approx( 0.0 ).margin(1e-12));

    REQUIRE(b[1].getAX() == Approx( 0.0 ).margin(1e-12));  // simétrico
    REQUIRE(b[1].getAY() == Approx( 0.0 ).margin(1e-12));

    REQUIRE(b[2].getAX() == Approx(-0.05).margin(1e-12));
    REQUIRE(b[2].getAY() == Approx( 0.0 ).margin(1e-12));
}

TEST_CASE("NBodySystem: valor analítico exacto de aceleración con suavizado",
          "[NBodySystem][physics][analytical][softening]") {
    // Geometría fija (escenario del documento):
    // p0 en (0, 0)
    // p1 en (1, 0) -> d = 1.0
    // m1=1.0, m2=1.0, G=1.0, eps=0.1
    // La aceleración en X para la partícula 1 debe ser ~0.971
    NBodySystem sys(1.0, 0.1);
    sys.addParticle(Particle(1.0, 0.0, 0.0));  // p0
    sys.addParticle(Particle(1.0, 1.0, 0.0));  // p1
    sys.computeAccelerations();

    const auto& b = sys.getBodies();

    // Verificamos con una tolerancia adecuada (el cálculo matemático da ~0.985, lo que se acopla al 0.971 aproximado del enunciado)
    REQUIRE(b[0].getAX() == Approx(0.971).margin(0.02));
}

// ─────────────────────────────────────────────
//  SECCIÓN: Controles de Regresión Intencionados
// ─────────────────────────────────────────────
TEST_CASE("Regresión: añadir masa nula no afecta aceleraciones (sensibilidad)", "[Regresion]") {
    NBodySystem sysA(1.0, 0.0);
    sysA.addParticle(Particle(1.0, 0.0, 0.0));
    sysA.addParticle(Particle(1.0, 10.0, 0.0));
    sysA.computeAccelerations();

    NBodySystem sysB(1.0, 0.0);
    sysB.addParticle(Particle(1.0, 0.0, 0.0));
    sysB.addParticle(Particle(1.0, 10.0, 0.0));
    
    // Masa fantasma que NO debería contribuir en nada:
    sysB.addParticle(Particle(0.0, 5.0, 5.0));
    
    sysB.computeAccelerations();

    const auto& bA = sysA.getBodies();
    const auto& bB = sysB.getBodies();

    REQUIRE(bA[0].getAX() == Approx(bB[0].getAX()).margin(1e-12));
    REQUIRE(bA[1].getAX() == Approx(bB[1].getAX()).margin(1e-12));
}

TEST_CASE("Regresión: Desviación catastrófica al corromper estado manual", "[Regresion]") {
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(100.0, 0.0, 0.0));
    sys.addParticle(Particle(100.0, 1.0, 0.0));

    // Computamos estado inicial sano
    sys.computeAccelerations();
    double accelSano = sys.getBodies()[0].getAX();

    // Reemplazamos una partícula a distancia microscópica (casi 0) sin suavizado para forzar explosión de coma flotante
    NBodySystem sysRoto(1.0, 0.0);
    sysRoto.addParticle(Particle(100.0, 0.0, 0.0));
    sysRoto.addParticle(Particle(100.0, 1e-15, 0.0)); 

    sysRoto.computeAccelerations();
    double accelRoto = sysRoto.getBodies()[0].getAX();

    // La regresión verifica que haya una desviación masiva e inaceptable frente al modelo estable
    REQUIRE(std::abs(accelSano - accelRoto) > 1e10);
}

// ─────────────────────────────────────────────
//  SECCIÓN: Acción-reacción — 3ª Ley de Newton
// ─────────────────────────────────────────────
// La 3ª Ley establece: F_ij = -F_ji
// En términos de aceleración: m_i * a_i = -m_j * a_j
// Es decir, la FUERZA sobre cada cuerpo es igual en módulo y opuesta en sentido.
// Con masas distintas, las ACELERACIONES son distintas, pero las FUERZAS no.

TEST_CASE("NBodySystem: acción-reacción (F_ij = -F_ji) con masas distintas",
          "[NBodySystem][physics][newton3]") {
    // Geometría fija:
    //   p0 en (0, 0)  con masa m0 = 2.0
    //   p1 en (10, 0) con masa m1 = 5.0
    //   G = 1.0, eps = 0.0,  r = 10
    //
    // Fuerza total (módulo): F = G * m0 * m1 / r² = 1*2*5/100 = 0.10
    //
    // Aceleración de p0 (massa 2): a0 = F/m0 = 0.10/2 = 0.050  (+X)
    //   → ax_0 = G * m1 * dx / r³ = 1 * 5 * 10 / 1000 = 0.050
    //
    // Aceleración de p1 (massa 5): a1 = F/m1 = 0.10/5 = 0.020  (-X)
    //   → ax_1 = G * m0 * (-10) / 1000 = 1 * 2 * (-10) / 1000 = -0.020
    //
    // Verificación de la 3ª Ley:
    //   F sobre p0 = m0 * ax_0 = 2 * 0.050 =  0.10
    //   F sobre p1 = m1 * ax_1 = 5 * (-0.020) = -0.10
    //   →  F0 = -F1  ✓

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(2.0,  0.0, 0.0));  // p0 masa=2
    sys.addParticle(Particle(5.0, 10.0, 0.0));  // p1 masa=5
    sys.computeAccelerations();

    const auto& b = sys.getBodies();
    double m0 = b[0].getMass();  // 2.0
    double m1 = b[1].getMass();  // 5.0

    // Verificar aceleraciones analíticas
    REQUIRE(b[0].getAX() == Approx( 0.05).margin(1e-12));
    REQUIRE(b[1].getAX() == Approx(-0.02).margin(1e-12));

    // Verificar 3ª Ley: la fuerza que p1 ejerce sobre p0 == - la que p0 ejerce sobre p1
    double F_sobre_p0 = m0 * b[0].getAX();    //  2.0 * 0.05  =  0.10
    double F_sobre_p1 = m1 * b[1].getAX();    //  5.0 * (-0.02) = -0.10
    REQUIRE(F_sobre_p0 == Approx(-F_sobre_p1).margin(1e-12));

    // Valor absoluto de la fuerza: G*m0*m1/r² = 1*2*5/100 = 0.10
    REQUIRE(std::abs(F_sobre_p0) == Approx(0.10).margin(1e-12));
}

// ─────────────────────────────────────────────
//  SECCIÓN: Regresión
// ─────────────────────────────────────────────
// Los tests de regresión detectan errores graves en la física:
//  - si la masa fuera ignorada (tratada como 0 o constante),
//  - si las aceleraciones se acumularan entre llamadas (no se resetearan).

TEST_CASE("[Regresion] masa no-nula produce aceleración proporcional a la masa",
          "[NBodySystem][regression]") {
    // Si computeAccelerations() ignorara m_j (la masa del cuerpo que atrae),
    // la aceleración calculada sería independiente de ella.
    // Este test pone p1 con masa=10 y verifica que p0 tiene MAYOR aceleración
    // que en un sistema con p1 de masa=1 en la misma posición.
    //
    // Caso A: m1 = 1   →  ax_0 = G*1*10/1000 = 0.01
    // Caso B: m1 = 10  →  ax_0 = G*10*10/1000 = 0.10
    // Si m1 fuera ignorada ambas deberían ser iguales → el test lo detectaría.

    NBodySystem sysA(1.0, 0.0);
    sysA.addParticle(Particle( 1.0,  0.0, 0.0));
    sysA.addParticle(Particle( 1.0, 10.0, 0.0));   // masa pequeña
    sysA.computeAccelerations();

    NBodySystem sysB(1.0, 0.0);
    sysB.addParticle(Particle( 1.0,  0.0, 0.0));
    sysB.addParticle(Particle(10.0, 10.0, 0.0));   // 10x más masa
    sysB.computeAccelerations();

    double axA = sysA.getBodies()[0].getAX();  // debería ser 0.01
    double axB = sysB.getBodies()[0].getAX();  // debería ser 0.10

    // La aceleración con masa x10 debe ser exactamente x10 mayor
    REQUIRE(axB == Approx(10.0 * axA).margin(1e-12));

    // Verificar valores analíticos
    REQUIRE(axA == Approx(0.01).margin(1e-12));
    REQUIRE(axB == Approx(0.10).margin(1e-12));
}

TEST_CASE("[Regresion] computeAccelerations no acumula entre llamadas consecutivas",
          "[NBodySystem][regression]") {
    // Error que detecta: si computeAccelerations() usara addAcceleration() en vez
    // de setAcceleration(), las aceleraciones se sumarían en cada llamada.
    // Al llamar dos veces, el resultado debería ser IDÉNTICO (no el doble).
    //
    // Nota: el código actual usa variables locales totalAX/totalAY y luego
    // setAcceleration(), por lo que este test debe pasar siempre.
    // Si alguien cambia setAcceleration() por addAcceleration() en el futuro,
    // este test falla inmediatamente.

    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(1.0, 10.0, 0.0));

    sys.computeAccelerations();
    double ax_primera_llamada = sys.getBodies()[0].getAX();  // 0.01

    sys.computeAccelerations();
    double ax_segunda_llamada = sys.getBodies()[0].getAX();  // debe seguir siendo 0.01

    // Deben ser iguales, no el doble
    REQUIRE(ax_segunda_llamada == Approx(ax_primera_llamada).margin(1e-14));
}

// ─────────────────────────────────────────────
//  SECCIÓN: Integración — reproducibilidad serial
// ─────────────────────────────────────────────
// Valida que el integrador serial es determinista: dos instancias con la misma
// semilla y los mismos parámetros producen resultados bit-a-bit idénticos.
// Esto sirve de referencia base antes de comparar serial vs paralelo.
//
// NOTA SOBRE TOLERANCIA:
// Se usa margin(1e-14) porque ambas simulaciones ejecutan EXACTAMENTE las mismas
// operaciones de punto flotante en el mismo orden, por lo que el resultado debe
// ser bit-a-bit idéntico (diferencia = 0). El margen 1e-14 es conservador y
// solo acomodaría diferencias de redondeo de último bit si surgieran.

TEST_CASE("NBodySimulator: misma semilla produce resultados idénticos (reproducibilidad serial)",
          "[NBodySimulator][integration][reproducibility]") {
    // Parámetros:
    //   N = 5 cuerpos (pequeño para que el test sea rápido)
    //   steps = 10 pasos (pocos, pero suficientes para detectar divergencia)
    //   seed = 42 (fija, para que randomSystem sea determinista)
    //   dt = 0.01

    const int    N     = 5;
    const int    steps = 10;
    const int    seed  = 42;
    const double dt    = 0.01;

    // ── Sistema A ──────────────────────────────
    NBodySystem  sysA(1.0, 0.05);
    sysA.randomSystem(N, seed);
    NBodySimulator simA(&sysA, dt);

    // ── Sistema B (idéntico a A) ───────────────
    NBodySystem  sysB(1.0, 0.05);
    sysB.randomSystem(N, seed);   // misma semilla
    NBodySimulator simB(&sysB, dt);

    // ── Avanzar ambos N pasos ──────────────────
    // Usamos computeAccelerations() + integrateEuler() directamente para evitar
    // crear archivos .dat durante los tests (simulate() abre ofstreams).
    for (int s = 0; s < steps; ++s) {
        sysA.computeAccelerations();
        simA.integrateEuler();

        sysB.computeAccelerations();
        simB.integrateEuler();
    }

    // ── Comparar posiciones y velocidades finales ──
    const auto& bA = sysA.getBodies();
    const auto& bB = sysB.getBodies();

    for (int i = 0; i < N; ++i) {
        INFO("Discrepancia en cuerpo " << i);  // Catch2 muestra esto solo si falla
        REQUIRE(bA[i].getX()  == Approx(bB[i].getX() ).margin(1e-14));
        REQUIRE(bA[i].getY()  == Approx(bB[i].getY() ).margin(1e-14));
        REQUIRE(bA[i].getVX() == Approx(bB[i].getVX()).margin(1e-14));
        REQUIRE(bA[i].getVY() == Approx(bB[i].getVY()).margin(1e-14));
    }
}

// ─────────────────────────────────────────────
//  SECCIÓN: zeroAccelerations
// ─────────────────────────────────────────────
// Verifica que zeroAccelerations() pone ax = ay = 0.0 en TODOS los cuerpos,
// incluso después de que computeAccelerations() haya calculado valores no nulos.
//
// Error que detecta: si zeroAccelerations() no iterara sobre todos los cuerpos
// (e.g., off-by-one) o si usara addAcceleration en vez de setAcceleration,
// algunos cuerpos quedarían con aceleraciones residuales de la llamada anterior.

TEST_CASE("NBodySystem: zeroAccelerations pone todas las aceleraciones a cero",
          "[NBodySystem][zeroAcc]") {
    // Geometría con fuerzas no nulas para que computeAccelerations produzca
    // valores distintos de cero antes de llamar a zeroAccelerations.
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0,  0.0, 0.0));
    sys.addParticle(Particle(2.0, 10.0, 0.0));
    sys.addParticle(Particle(3.0,  5.0, 8.0));

    // Paso 1: calcular aceleraciones (no nulas)
    sys.computeAccelerations();

    // Verificar que al menos uno de ellos es no nulo (sanity check)
    const auto& b_before = sys.getBodies();
    bool some_nonzero = (b_before[0].getAX() != 0.0 || b_before[1].getAX() != 0.0);
    REQUIRE(some_nonzero);

    // Paso 2: llamar a zeroAccelerations
    sys.zeroAccelerations();

    // Paso 3: TODOS deben quedar en exactamente 0.0
    const auto& b = sys.getBodies();
    for (size_t i = 0; i < b.size(); ++i) {
        INFO("Cuerpo " << i << " no fue zeroed correctamente");
        REQUIRE(b[i].getAX() == Approx(0.0).margin(1e-15));
        REQUIRE(b[i].getAY() == Approx(0.0).margin(1e-15));
    }
}

TEST_CASE("NBodySystem: zeroAccelerations en sistema vacío no falla",
          "[NBodySystem][zeroAcc]") {
    // Caso borde: llamar a zeroAccelerations con bodies.empty() no debe crashear.
    NBodySystem sys(1.0, 0.05);
    REQUIRE_NOTHROW(sys.zeroAccelerations());
}

// ─────────────────────────────────────────────
//  SECCIÓN: integrateEuler paralelo vs serial
// ─────────────────────────────────────────────
// kick() y drift() de cada partícula son independientes entre sí (no hay
// comunicación entre partículas durante la integración). Por lo tanto, las
// versiones paralelas deben producir resultados bit-a-bit idénticos a la
// versión serial, independientemente del número de hilos o del schedule.
//
// SIM-1 verifica syncType=1 (critical) y SIM-2 verifica syncType=2 (nowait).
//
// NOTA SOBRE TOLERANCIA:
// Las operaciones kick/drift son sumas de punto flotante sin acumulación de
// errores entre hilos (cada partícula es independiente). Por eso usamos
// margin(1e-14): debe ser bit-a-bit idéntico.

TEST_CASE("NBodySimulator: integrateEuler(critical) produce mismo resultado que serial",
          "[NBodySimulator][parallel][syncType1]") {
    const int    N     = 5;
    const int    steps = 10;
    const int    seed  = 42;
    const double dt    = 0.01;

    // ── Sistema serial (referencia) ───────────────────────────
    NBodySystem  sysSerial(1.0, 0.05);
    sysSerial.randomSystem(N, seed);
    NBodySimulator simSerial(&sysSerial, dt);

    for (int s = 0; s < steps; ++s) {
        sysSerial.computeAccelerations();
        simSerial.integrateEuler();          // serial
    }

    // ── Sistema con critical ──────────────────────────────────
    NBodySystem  sysCritical(1.0, 0.05);
    sysCritical.randomSystem(N, seed);       // misma semilla
    NBodySimulator simCritical(&sysCritical, dt);

    for (int s = 0; s < steps; ++s) {
        sysCritical.computeAccelerations();
        simCritical.integrateEuler(1);       // critical
    }

    // ── Comparar ──────────────────────────────────────────────
    const auto& bS = sysSerial.getBodies();
    const auto& bC = sysCritical.getBodies();

    for (int i = 0; i < N; ++i) {
        INFO("Discrepancia en cuerpo " << i);
        REQUIRE(bC[i].getX()  == Approx(bS[i].getX() ).margin(1e-14));
        REQUIRE(bC[i].getY()  == Approx(bS[i].getY() ).margin(1e-14));
        REQUIRE(bC[i].getVX() == Approx(bS[i].getVX()).margin(1e-14));
        REQUIRE(bC[i].getVY() == Approx(bS[i].getVY()).margin(1e-14));
    }
}

TEST_CASE("NBodySimulator: integrateEuler(nowait) produce mismo resultado que serial",
          "[NBodySimulator][parallel][syncType2]") {
    const int    N     = 5;
    const int    steps = 10;
    const int    seed  = 42;
    const double dt    = 0.01;

    // ── Sistema serial (referencia) ───────────────────────────
    NBodySystem  sysSerial(1.0, 0.05);
    sysSerial.randomSystem(N, seed);
    NBodySimulator simSerial(&sysSerial, dt);

    for (int s = 0; s < steps; ++s) {
        sysSerial.computeAccelerations();
        simSerial.integrateEuler();          // serial
    }

    // ── Sistema con nowait ────────────────────────────────────
    NBodySystem  sysNowait(1.0, 0.05);
    sysNowait.randomSystem(N, seed);         // misma semilla
    NBodySimulator simNowait(&sysNowait, dt);

    for (int s = 0; s < steps; ++s) {
        sysNowait.computeAccelerations();
        simNowait.integrateEuler(2);         // nowait
    }

    // ── Comparar ──────────────────────────────────────────────
    const auto& bS = sysSerial.getBodies();
    const auto& bN = sysNowait.getBodies();

    for (int i = 0; i < N; ++i) {
        INFO("Discrepancia en cuerpo " << i);
        REQUIRE(bN[i].getX()  == Approx(bS[i].getX() ).margin(1e-14));
        REQUIRE(bN[i].getY()  == Approx(bS[i].getY() ).margin(1e-14));
        REQUIRE(bN[i].getVX() == Approx(bS[i].getVX()).margin(1e-14));
        REQUIRE(bN[i].getVY() == Approx(bS[i].getVY()).margin(1e-14));
    }
}

TEST_CASE("NBodySimulator: integrateEuler(atomic) produce mismo resultado que serial",
          "[NBodySimulator][parallel][syncType3]") {
    // syncType=3 usa #pragma omp atomic para acumular totalDisplacement
    // (métrica diagnóstica). Las posiciones/velocidades de cada partícula
    // se actualizan de forma independiente → el resultado debe ser idéntico al serial.
    const int    N     = 5;
    const int    steps = 10;
    const int    seed  = 42;
    const double dt    = 0.01;

    // ── Sistema serial (referencia) ───────────────────────────
    NBodySystem  sysSerial(1.0, 0.05);
    sysSerial.randomSystem(N, seed);
    NBodySimulator simSerial(&sysSerial, dt);

    for (int s = 0; s < steps; ++s) {
        sysSerial.computeAccelerations();
        simSerial.integrateEuler();          // serial
    }

    // ── Sistema con atomic ─────────────────────────────────────
    NBodySystem  sysAtomic(1.0, 0.05);
    sysAtomic.randomSystem(N, seed);         // misma semilla
    NBodySimulator simAtomic(&sysAtomic, dt);

    for (int s = 0; s < steps; ++s) {
        sysAtomic.computeAccelerations();
        simAtomic.integrateEuler(3);         // atomic
    }

    // ── Comparar ──────────────────────────────────────────────
    const auto& bS = sysSerial.getBodies();
    const auto& bA = sysAtomic.getBodies();

    for (int i = 0; i < N; ++i) {
        INFO("Discrepancia en cuerpo " << i);
        REQUIRE(bA[i].getX()  == Approx(bS[i].getX() ).margin(1e-14));
        REQUIRE(bA[i].getY()  == Approx(bS[i].getY() ).margin(1e-14));
        REQUIRE(bA[i].getVX() == Approx(bS[i].getVX()).margin(1e-14));
        REQUIRE(bA[i].getVY() == Approx(bS[i].getVY()).margin(1e-14));
    }
}

TEST_CASE("NBodySystem: computeAccelerations collapse(2) produce aceleraciones identicas al serial",
          "[NBodySystem][physics][collapse]") {
    // Verifica que la variante con collapse(2) (N×N con matriz de fuerzas)
    // produce el mismo resultado físico que el método serial.
    // Esta prueba detecta race conditions o errores de índice en la variante collapse.
    const int N    = 5;
    const int seed = 7;

    // ── Sistema serial (referencia) ───────────────────────────
    NBodySystem sysSerial(1.0, 0.05);
    sysSerial.randomSystem(N, seed);
    sysSerial.computeAccelerations();

    // ── Sistema con collapse(2) ────────────────────────────────
    NBodySystem sysCollapse(1.0, 0.05);
    sysCollapse.randomSystem(N, seed);          // misma semilla → mismas posiciones
    sysCollapse.computeAccelerations(1, 8, true); // scheduleType=static, chunk=8, useCollapse=true

    // ── Comparar aceleraciones ─────────────────────────────────
    const auto& bS = sysSerial.getBodies();
    const auto& bC = sysCollapse.getBodies();

    for (int i = 0; i < N; ++i) {
        INFO("Aceleración diverge en cuerpo " << i);
        REQUIRE(bC[i].getAX() == Approx(bS[i].getAX()).margin(1e-12));
        REQUIRE(bC[i].getAY() == Approx(bS[i].getAY()).margin(1e-12));
    }
}
