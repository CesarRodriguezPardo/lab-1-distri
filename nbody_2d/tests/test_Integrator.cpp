#include "catch.hpp"
#include "../Integrator.h"
#include "../NBodySystem.h"
#include "../Particle.h"
#include <cmath>

TEST_CASE("Integrator: Integración Serial desplaza partículas de forma predecible", "[Integrator][serial]") {
    NBodySystem sys(1.0, 0.0);
    sys.addParticle(Particle(1.0, 0.0, 0.0)); // p0
    sys.addParticle(Particle(1.0, 10.0, 0.0)); // p1

    // Computa fuerza analíticamente conocida (para que tengan a!=0)
    sys.computeAccelerations();

    Integrator integrator(&sys, 0.1); // dt = 0.1
    integrator.integrateEuler();

    const auto& b = sys.getBodies();
    // p0: m=1.0, pos(0,0). Fuerza de p1 en(10,0) -> ax = 1 / 1000 = 0.01
    // vx = 0 + 0.01 * 0.1 = 0.001
    // x = 0 + 0.001 * 0.1 = 0.0001
    REQUIRE(b[0].getVX() == Approx(0.001).margin(1e-12));
    REQUIRE(b[0].getX() == Approx(0.0001).margin(1e-12));
}

TEST_CASE("Integrator: Integración Paralela (critical) debe coincidir con Serial", "[Integrator][parallel]") {
    NBodySystem sysSerial(1.0, 0.0);
    sysSerial.addParticle(Particle(1.0, 0.0, 0.0));
    sysSerial.addParticle(Particle(1.0, 5.0, 0.0));

    NBodySystem sysCritical(1.0, 0.0);
    sysCritical.addParticle(Particle(1.0, 0.0, 0.0));
    sysCritical.addParticle(Particle(1.0, 5.0, 0.0));

    sysSerial.computeAccelerations();
    sysCritical.computeAccelerations();

    Integrator intSerial(&sysSerial, 0.1);
    Integrator intCritical(&sysCritical, 0.1);

    intSerial.integrateEuler();
    intCritical.integrateEuler(1); // syncType = 1 (critical)

    const auto& bS = sysSerial.getBodies();
    const auto& bC = sysCritical.getBodies();

    for (size_t i = 0; i < bS.size(); ++i) {
        REQUIRE(bS[i].getX() == Approx(bC[i].getX()).margin(1e-12));
        REQUIRE(bS[i].getVX() == Approx(bC[i].getVX()).margin(1e-12));
    }
}

TEST_CASE("Integrator: Integración Paralela (nowait y atomic) debe coincidir con Serial", "[Integrator][parallel]") {
    NBodySystem sysSerial(1.0, 0.0);
    sysSerial.addParticle(Particle(1.0, 0.0, 0.0));
    sysSerial.addParticle(Particle(1.0, 5.0, 0.0));

    NBodySystem sysNoWait(1.0, 0.0);
    sysNoWait.addParticle(Particle(1.0, 0.0, 0.0));
    sysNoWait.addParticle(Particle(1.0, 5.0, 0.0));

    NBodySystem sysAtomic(1.0, 0.0);
    sysAtomic.addParticle(Particle(1.0, 0.0, 0.0));
    sysAtomic.addParticle(Particle(1.0, 5.0, 0.0));

    sysSerial.computeAccelerations();
    sysNoWait.computeAccelerations();
    sysAtomic.computeAccelerations();

    Integrator intSerial(&sysSerial, 0.1);
    Integrator intNoWait(&sysNoWait, 0.1);
    Integrator intAtomic(&sysAtomic, 0.1);

    intSerial.integrateEuler();
    intNoWait.integrateEuler(2, false); // syncType = 2 (nowait), no-barrier
    intAtomic.integrateEuler(3); // syncType = 3 (atomic)

    const auto& bS = sysSerial.getBodies();
    const auto& bNW = sysNoWait.getBodies();
    const auto& bA = sysAtomic.getBodies();

    for (size_t i = 0; i < bS.size(); ++i) {
        REQUIRE(bNW[i].getX() == Approx(bS[i].getX()).margin(1e-12));
        REQUIRE(bA[i].getX() == Approx(bS[i].getX()).margin(1e-12));
    }
}
