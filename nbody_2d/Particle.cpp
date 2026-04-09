#include "Particle.h"

Particle::Particle(double m, double x0, double y0)
    : mass(m), x(x0), y(y0), vx(0.0), vy(0.0), ax(0.0), ay(0.0) {}

void Particle::setAcceleration(double ax_, double ay_){
    ax = ax_;
    ay = ay_;
}

void Particle::addAcceleration(double d_ax, double d_ay){
    ax += d_ax;
    ay += d_ay;
}

void Particle::kick(double dt){
    vx += ax * dt;
    vy += ay * dt;
}

void Particle::drift(double dt){
    x += vx * dt;
    y += vy * dt;
}
