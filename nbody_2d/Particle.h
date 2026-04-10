#ifndef PARTICLE_H
#define PARTICLE_H
#include <iostream>

class Particle {
    private:
        double mass; //
        double x, y; // Posicion
        double vx, vy; // Velocidad
        double ax, ay; // Aceleracion
    public:
        Particle(double m, double x0, double y0);

        void addAcceleration(double d_ax, double d_ay);
        void kick(double dt); // v = v + a * dt
        void drift(double dt); // r = v * dt

        // getters
        double getMass() const { return mass; }
        double getX() const { return x; }
        double getY() const { return y; }
        double getVX() const { return vx; }
        double getVY() const { return vy; }
        double getAX() const { return ax; }
        double getAY() const { return ay; }

        //setters
        void setPosition(double x_, double y_);
        void setVelocity(double vx_, double vy_);
        void setAcceleration(double ax_, double ay_);
};

#endif