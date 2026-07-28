#include "Integrator.h"
#include <cmath>


Integrator::Integrator(NBodySystem* sys, double dt)
    : system(sys), time_step(dt) {}

// ─────────────────────────────────────────────────────────────
//  Variante 1 — Serial
//  Euler simple: kick (v = v + a·dt) + drift (r = r + v·dt)
// ─────────────────────────────────────────────────────────────
void Integrator::integrateEuler() {
    auto& particles = system->getParticles();
    int n = static_cast<int>(particles.size());

    for (int i = 0; i < n; ++i) {
        particles[i].kick(time_step);
        particles[i].drift(time_step);
    }
}

// ─────────────────────────────────────────────────────────────
//  Variante 2 — Paralelo con tipo de sincronización
//   syncType = 1 → omp critical  (serializa escrituras)
//   syncType = 2 → omp for nowait (sin barrera implícita al final)
//   default      → delega a serial
// ─────────────────────────────────────────────────────────────
void Integrator::integrateEuler(int syncType) {
    auto& particles = system->getParticles();
    int n = static_cast<int>(particles.size());

    switch (syncType) {
    case 1: // critical — demuestra el uso de la cláusula critical
        #pragma omp parallel for schedule(static)
        for (int i = 0; i < n; ++i) {
            #pragma omp critical
            {
                particles[i].kick(time_step);
                particles[i].drift(time_step);
            }
        }
        break;

    case 2: // nowait — las partículas i son independientes entre sí,
            //          por lo que eliminar la barrera implícita es seguro
        #pragma omp parallel
        {
            #pragma omp for nowait schedule(static)
            for (int i = 0; i < n; ++i) {
                particles[i].kick(time_step);
                particles[i].drift(time_step);
            }
        }
        break;

    case 3: // atomic — integra partículas en paralelo y acumula de forma atómica
            // el desplazamiento total del paso (métrica diagnóstica compartida).
            // Las escrituras por partícula son seguras (accesos independientes);
            // la acumulación de 'totalDisplacement' sí requiere protección atómica
            // porque múltiples hilos escriben en la misma variable.
        {
            double totalDisplacement = 0.0;
            #pragma omp parallel for schedule(static)
            for (int i = 0; i < n; ++i) {
                particles[i].kick(time_step);
                double dx = particles[i].getVX() * time_step;
                double dy = particles[i].getVY() * time_step;
                particles[i].drift(time_step);
                double disp = std::sqrt(dx * dx + dy * dy);
                #pragma omp atomic
                totalDisplacement += disp;
            }
            (void)totalDisplacement; // disponible para logging externo
        }
        break;

    default:
        integrateEuler(); // fallback serial
        break;
    }
}

// ─────────────────────────────────────────────────────────────
//  Variante 3 — Paralelo nowait con control de barrier
//   Solo activo cuando syncType == 2 (nowait).
//   use_barrier = true  → inserta #pragma omp barrier explícito
//   use_barrier = false → nowait puro (sin sincronización final)
// ─────────────────────────────────────────────────────────────
void Integrator::integrateEuler(int syncType, bool use_barrier) {
    if (syncType != 2) {
        integrateEuler(syncType); // delega a variante 2
        return;
    }

    auto& particles = system->getParticles();
    int n = static_cast<int>(particles.size());

    #pragma omp parallel
    {
        #pragma omp for nowait schedule(static)
        for (int i = 0; i < n; ++i) {
            particles[i].kick(time_step);
            particles[i].drift(time_step);
        }

        if (use_barrier) {
            // barrier explícito: todos los hilos sincronizan aquí
            // antes de continuar con el siguiente paso de la simulación
            #pragma omp barrier
        }
    }
}


void Integrator::integrateEulerGpu(CudaBuffer* buffer) {
    // ---------------------------------------------------------
    // PASO 1: Sincronización explícita
    // Esperamos a que el kernel de aceleraciones (Rol 1) termine.
    // ---------------------------------------------------------
    cudaDeviceSynchronize();

    // Obtenemos las partículas y la cantidad N
    auto& particles = system->getParticles();
    int n = static_cast<int>(particles.size());
    size_t bytes = n * sizeof(double);

    // ---------------------------------------------------------
    // PASO 2: Device a Host (SoA -> Cajas temporales)
    // ---------------------------------------------------------
    std::vector<double> h_ax(n);
    std::vector<double> h_ay(n);

    // cudaMemcpy(destino, origen, tamaño_bytes, dirección)
    cudaMemcpy(h_ax.data(), buffer->d_ax, bytes, cudaMemcpyDeviceToHost);
    cudaMemcpy(h_ay.data(), buffer->d_ay, bytes, cudaMemcpyDeviceToHost);

    // ---------------------------------------------------------
    // PASO 3: Ensamblar e Integrar (Tu Euler en la CPU)
    // Inyectamos datos SoA al objeto AoS y aplicamos física
    // ---------------------------------------------------------
    for (int i = 0; i < n; ++i) {
        // Pasamos la aceleración calculada en GPU al objeto Particle
        particles[i].setAcceleration(h_ax[i], h_ay[i]);
        
        // Ejecutamos kick y drift en el Host
        particles[i].kick(time_step);
        particles[i].drift(time_step);
    }

    // ---------------------------------------------------------
    // PASO 4: Desempaquetar (AoS -> Cajas temporales)
    // ---------------------------------------------------------
    std::vector<double> h_x(n);
    std::vector<double> h_y(n);

    for (int i = 0; i < n; ++i) {
        h_x[i] = particles[i].getX();
        h_y[i] = particles[i].getY();
    }

    // Host a Device (Enviar posiciones actualizadas a la GPU)
    cudaMemcpy(buffer->d_x, h_x.data(), bytes, cudaMemcpyHostToDevice);
    cudaMemcpy(buffer->d_y, h_y.data(), bytes, cudaMemcpyHostToDevice);
}