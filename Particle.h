#ifndef PARTICLE_H
#define PARTICLE_H

class Particle {
    private:
        double mass; //
        double x, y; // Posicion
        double vx, vy; // Velocidad
        double ax, ay; // Aceleracion
    public:
        Particle(double m, double x0, double y0);
        void setAcceleration(double ax_, double ay_);
        void addAcceleration(double d_ax, double d_ay);
        void kick(double dt); // v = v + a * dt
        void drift(double dt); // r = v * dt
};

#endif