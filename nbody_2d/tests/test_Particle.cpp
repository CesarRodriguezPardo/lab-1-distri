// test_Particle.cpp
// Pruebas UNITARIAS para la clase Particle.
// Usa Catch2 v3 (catch_amalgamated.hpp).
// Cada TEST_CASE es independiente: crea su propia Particle desde cero.
// Se usa Approx() para comparaciones de punto flotante con tolerancia.

#include "catch_amalgamated.hpp"
#include "../Particle.h"

using Catch::Approx;

// ─────────────────────────────────────────────
//  SECCIÓN: Constructor
// ─────────────────────────────────────────────

TEST_CASE("Particle: el constructor inicializa todos los campos correctamente", "[Particle][constructor]") {
    Particle p(5.0, 1.0, 2.0);

    REQUIRE(p.getMass() == Approx(5.0));
    REQUIRE(p.getX()    == Approx(1.0));
    REQUIRE(p.getY()    == Approx(2.0));

    // Velocidad y aceleración deben arrancar en cero
    REQUIRE(p.getVX() == Approx(0.0));
    REQUIRE(p.getVY() == Approx(0.0));
    REQUIRE(p.getAX() == Approx(0.0));
    REQUIRE(p.getAY() == Approx(0.0));
}

// ─────────────────────────────────────────────
//  SECCIÓN: kick  (v = v + a * dt)
// ─────────────────────────────────────────────

TEST_CASE("Particle: kick() actualiza la velocidad con v = v + a*dt", "[Particle][kick]") {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(2.0, -3.0);
    p.kick(0.5);

    // vx = 0 + 2.0 * 0.5 = 1.0
    REQUIRE(p.getVX() == Approx(1.0));
    // vy = 0 + (-3.0) * 0.5 = -1.5
    REQUIRE(p.getVY() == Approx(-1.5));
}

TEST_CASE("Particle: kick() con dt=0 no cambia la velocidad", "[Particle][kick]") {
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(10.0, 10.0);
    p.kick(0.0);

    REQUIRE(p.getVX() == Approx(0.0));
    REQUIRE(p.getVY() == Approx(0.0));
}

// ─────────────────────────────────────────────
//  SECCIÓN: drift  (r = r + v * dt)
// ─────────────────────────────────────────────

TEST_CASE("Particle: drift() actualiza la posición con r = r + v*dt", "[Particle][drift]") {
    Particle p(1.0, 0.0, 0.0);
    p.setVelocity(4.0, -2.0);
    p.drift(0.25);

    // x = 0 + 4.0 * 0.25 = 1.0
    REQUIRE(p.getX() == Approx(1.0));
    // y = 0 + (-2.0) * 0.25 = -0.5
    REQUIRE(p.getY() == Approx(-0.5));
}

TEST_CASE("Particle: drift() con dt=0 no cambia la posición", "[Particle][drift]") {
    Particle p(1.0, 3.0, 7.0);
    p.setVelocity(100.0, 100.0);
    p.drift(0.0);

    REQUIRE(p.getX() == Approx(3.0));
    REQUIRE(p.getY() == Approx(7.0));
}

// ─────────────────────────────────────────────
//  SECCIÓN: Setters
// ─────────────────────────────────────────────

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

    // ax = 1.0 + 0.5 = 1.5
    REQUIRE(p.getAX() == Approx(1.5));
    // ay = 2.0 + (-1.0) = 1.0
    REQUIRE(p.getAY() == Approx(1.0));
}

TEST_CASE("Particle: setPosition() y setVelocity() funcionan correctamente", "[Particle][setters]") {
    Particle p(1.0, 0.0, 0.0);
    p.setPosition(10.0, -5.0);
    p.setVelocity(-3.0, 4.0);

    REQUIRE(p.getX()  == Approx(10.0));
    REQUIRE(p.getY()  == Approx(-5.0));
    REQUIRE(p.getVX() == Approx(-3.0));
    REQUIRE(p.getVY() == Approx(4.0));
}

// ─────────────────────────────────────────────
//  SECCIÓN: Secuencia completa kick + drift
// ─────────────────────────────────────────────

TEST_CASE("Particle: secuencia kick + drift produce movimiento correcto (Euler step)", "[Particle][integration]") {
    // Simula un paso del integrador de Euler:
    // Partícula en y=0, ay=9.8 (gravedad), dt=1.0
    Particle p(1.0, 0.0, 0.0);
    p.setAcceleration(0.0, 9.8);

    p.kick(1.0);   // vy = 0 + 9.8*1 = 9.8
    p.drift(1.0);  // y  = 0 + 9.8*1 = 9.8

    REQUIRE(p.getVY() == Approx(9.8));
    REQUIRE(p.getY()  == Approx(9.8));
}
