#include "Particle.h"

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

Particle::Particle(double m, double x0, double y0, double vx0, double vy0)
    : mass(m), x(x0), y(y0), vx(vx0), vy(vy0), ax(0.0), ay(0.0) {}

// -----------------------------------------------------------------------------
// Acceleration management
// -----------------------------------------------------------------------------

void Particle::zeroAcceleration() {
    ax = 0.0;
    ay = 0.0;
}

void Particle::addAcceleration(double dax, double day) {
    ax += dax;
    ay += day;
}

void Particle::setAcceleration(double ax_, double ay_) {
    ax = ax_;
    ay = ay_;
}

// -----------------------------------------------------------------------------
// Euler integrator steps
// -----------------------------------------------------------------------------

/**
 * Kick: v_i <- v_i + a_i * dt
 * Updates velocity using the current acceleration.
 * Must be called AFTER computeAccelerations() and BEFORE drift().
 */
void Particle::kick(double dt) {
    vx += ax * dt;
    vy += ay * dt;
}

/**
 * Drift: r_i <- r_i + v_i * dt
 * Updates position using the current (post-kick) velocity.
 * Must be called AFTER kick().
 */
void Particle::drift(double dt) {
    x += vx * dt;
    y += vy * dt;
}

// -----------------------------------------------------------------------------
// Getters
// -----------------------------------------------------------------------------

double Particle::getMass() const { return mass; }
double Particle::getX()    const { return x;    }
double Particle::getY()    const { return y;    }
double Particle::getVx()   const { return vx;   }
double Particle::getVy()   const { return vy;   }
double Particle::getAx()   const { return ax;   }
double Particle::getAy()   const { return ay;   }

// -----------------------------------------------------------------------------
// Setters
// -----------------------------------------------------------------------------

void Particle::setPosition(double x_, double y_) {
    x = x_;
    y = y_;
}

void Particle::setVelocity(double vx_, double vy_) {
    vx = vx_;
    vy = vy_;
}