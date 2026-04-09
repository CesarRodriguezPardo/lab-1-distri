#pragma once

/**
 * @file Particle.h
 * @brief Representa una partícula individual en la simulación gravitatoria N-cuerpos 2D.
 *
 * Cada partícula almacena masa, posición (x, y), velocidad (vx, vy) y aceleración (ax, ay).
 * Las operaciones kick y drift implementan el integrador de Euler explícito:
 *   kick:  v_i <- v_i + a_i * dt
 *   drift: r_i <- r_i + v_i * dt
 *
 * Rol: Modelo y datos (Semana 1)
 */
class Particle {
private:
    double mass;   ///< Masa de la partícula (debe ser > 0)
    double x, y;   ///< Posición en el plano 2D
    double vx, vy; ///< Velocidad en el plano 2D
    double ax, ay; ///< Aceleración en el plano 2D (se recalcula cada paso)

public:
    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    /**
     * @brief Construye una partícula con estado inicial dado.
     * @param m   Masa (debe ser > 0; no se admiten masas nulas ni negativas)
     * @param x0  Posición inicial en x
     * @param y0  Posición inicial en y
     * @param vx0 Velocidad inicial en x (por defecto 0)
     * @param vy0 Velocidad inicial en y (por defecto 0)
     */
    Particle(double m, double x0, double y0,
             double vx0 = 0.0, double vy0 = 0.0);

    // -------------------------------------------------------------------------
    // Gestión de aceleración
    // -------------------------------------------------------------------------

    /**
     * @brief Pone la aceleración a (0, 0).
     * Debe llamarse al inicio de cada paso temporal, antes de acumular fuerzas.
     */
    void zeroAcceleration();

    /**
     * @brief Acumula una contribución incremental a la aceleración.
     * Usado al sumar la contribución de cada cuerpo j sobre este cuerpo i.
     * @param dax Incremento en la componente x de la aceleración
     * @param day Incremento en la componente y de la aceleración
     */
    void addAcceleration(double dax, double day);

    /**
     * @brief Asigna directamente la aceleración.
     * @param ax_ Nueva componente x de la aceleración
     * @param ay_ Nueva componente y de la aceleración
     */
    void setAcceleration(double ax_, double ay_);

    // -------------------------------------------------------------------------
    // Pasos del integrador de Euler
    // -------------------------------------------------------------------------

    /**
     * @brief Kick: actualiza la velocidad con la aceleración actual.
     * v_i <- v_i + a_i * dt
     * @param dt Paso temporal
     */
    void kick(double dt);

    /**
     * @brief Drift: actualiza la posición con la velocidad actual.
     * r_i <- r_i + v_i * dt
     * Debe llamarse DESPUÉS del kick para el orden Euler estándar.
     * @param dt Paso temporal
     */
    void drift(double dt);

    // -------------------------------------------------------------------------
    // Getters (const)
    // -------------------------------------------------------------------------
    double getMass() const; ///< Retorna la masa
    double getX()    const; ///< Retorna la coordenada x
    double getY()    const; ///< Retorna la coordenada y
    double getVx()   const; ///< Retorna la velocidad en x
    double getVy()   const; ///< Retorna la velocidad en y
    double getAx()   const; ///< Retorna la aceleración en x
    double getAy()   const; ///< Retorna la aceleración en y

    // -------------------------------------------------------------------------
    // Setters
    // -------------------------------------------------------------------------
    /**
     * @brief Asigna directamente la posición.
     * @param x_ Nueva coordenada x
     * @param y_ Nueva coordenada y
     */
    void setPosition(double x_, double y_);

    /**
     * @brief Asigna directamente la velocidad.
     * @param vx_ Nueva velocidad en x
     * @param vy_ Nueva velocidad en y
     */
    void setVelocity(double vx_, double vy_);
};