#include "MetricsCalculator.h"
#include <cmath>
#include <limits>
#include <omp.h>
#include <iostream>
#include <fstream>
#include <iomanip>

MetricsCalculator::MetricsCalculator(NBodySystem* sys) : system(sys), 
    kineticEnergy(0), potentialEnergy(0), totalEnergy(0), 
    momentumX(0), momentumY(0), momentumMagnitude(0),
    cmX(0), cmY(0), rmsRadius(0), minDistance(std::numeric_limits<double>::max()) {}

void MetricsCalculator::calculateEnergy(int method) {
    const auto& bodies = system->getBodies();
    int n = bodies.size();
    double G = system->getG_const();
    double eps = system->getEps();

    double K = 0.0;
    double U = 0.0;

    if (method == 0) {
        // --- Método 0: reduction (OpenMP nativo) ---
        #pragma omp parallel for reduction(+:K)
        for (int i = 0; i < n; ++i) {
            double vx = bodies[i].getVX();
            double vy = bodies[i].getVY();
            K += 0.5 * bodies[i].getMass() * (vx * vx + vy * vy);
        }

        #pragma omp parallel for reduction(+:U) schedule(dynamic)
        for (int i = 0; i < n; ++i) {
            double u_local = 0.0;
            for (int j = i + 1; j < n; ++j) {
                double dx = bodies[j].getX() - bodies[i].getX();
                double dy = bodies[j].getY() - bodies[i].getY();
                double distSq = dx * dx + dy * dy + eps * eps;
                u_local += (bodies[i].getMass() * bodies[j].getMass()) / std::sqrt(distSq);
            }
            U -= G * u_local;
        }
    } 
    else if (method == 1) {
        // --- Método 1: atomic ---
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            double vx = bodies[i].getVX();
            double vy = bodies[i].getVY();
            double k_i = 0.5 * bodies[i].getMass() * (vx * vx + vy * vy);
            #pragma omp atomic
            K += k_i;
        }

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                double dx = bodies[j].getX() - bodies[i].getX();
                double dy = bodies[j].getY() - bodies[i].getY();
                double distSq = dx * dx + dy * dy + eps * eps;
                double u_ij = -G * (bodies[i].getMass() * bodies[j].getMass()) / std::sqrt(distSq);
                #pragma omp atomic
                U += u_ij;
            }
        }
    }
    else if (method == 2) {
        // --- Método 2: critical ---
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            double vx = bodies[i].getVX();
            double vy = bodies[i].getVY();
            double k_i = 0.5 * bodies[i].getMass() * (vx * vx + vy * vy);
            #pragma omp critical(kinetic)
            K += k_i;
        }

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < n; ++i) {
            double u_row = 0.0;
            for (int j = i + 1; j < n; ++j) {
                double dx = bodies[j].getX() - bodies[i].getX();
                double dy = bodies[j].getY() - bodies[i].getY();
                double distSq = dx * dx + dy * dy + eps * eps;
                u_row -= G * (bodies[i].getMass() * bodies[j].getMass()) / std::sqrt(distSq);
            }
            #pragma omp critical(potential)
            U += u_row;
        }
    }

    kineticEnergy = K;
    potentialEnergy = U;
    totalEnergy = K + U;
}

void MetricsCalculator::calculateMetricsFirstprivate() {
    const auto& bodies = system->getBodies();
    int n = bodies.size();
    
    double totalMass = 0.0;
    double sumMx = 0.0;
    double sumMy = 0.0;
    double px = 0.0;
    double py = 0.0;
    
    // Demostrando firstprivate con un contador interno (aunque no afecte sumas)
    int step_counter = 1; 
    
    #pragma omp parallel for reduction(+:totalMass, sumMx, sumMy, px, py) firstprivate(step_counter)
    for(int i = 0; i < n; ++i) {
        // Cada thread comienza con step_counter = 1
        step_counter++;
        
        double m = bodies[i].getMass();
        double x = bodies[i].getX();
        double y = bodies[i].getY();
        double vx = bodies[i].getVX();
        double vy = bodies[i].getVY();
        
        totalMass += m;
        sumMx += m * x;
        sumMy += m * y;
        px += m * vx;
        py += m * vy;
    }
    
    if (totalMass > 0) {
        cmX = sumMx / totalMass;
        cmY = sumMy / totalMass;
    }
    momentumX = px;
    momentumY = py;
    momentumMagnitude = std::sqrt(px*px + py*py);
}

void MetricsCalculator::calculateFinalStateLastprivate() {
    const auto& bodies = system->getBodies();
    int n = bodies.size();

    double sumSqDist = 0.0;
    double totalMass = 0.0;

    double lastX = 0.0, lastY = 0.0;

    #pragma omp parallel for reduction(+:sumSqDist, totalMass) lastprivate(lastX, lastY)
    for(int i = 0; i < n; ++i) {
        double m  = bodies[i].getMass();
        double dx = bodies[i].getX() - cmX;
        double dy = bodies[i].getY() - cmY;

        sumSqDist += m * (dx * dx + dy * dy);
        totalMass += m;

        // La última iteración del loop deja sus valores en lastX/lastY (lastprivate)
        lastX = bodies[i].getX();
        lastY = bodies[i].getY();
    }
    // Aquí lastX/lastY contienen la posición de la partícula n-1 (última iteración)
    (void)lastX; (void)lastY; // Usadas conceptualmente; evita warning de unused

    if (totalMass > 0) {
        rmsRadius = std::sqrt(sumSqDist / totalMass);
    }

    // Cálculo de distancia mínima — carga triangular.
    // schedule(guided): el runtime reduce progresivamente el tamaño del chunk,
    // lo que se adapta bien al trabajo decreciente del bucle i<j.
    double min_dSq = std::numeric_limits<double>::max();
    #pragma omp parallel for reduction(min:min_dSq) schedule(guided)
    for(int i = 0; i < n; ++i) {
        for(int j = i + 1; j < n; ++j) {
            double dx = bodies[j].getX() - bodies[i].getX();
            double dy = bodies[j].getY() - bodies[i].getY();
            double dSq = dx * dx + dy * dy;
            if(dSq < min_dSq) {
                min_dSq = dSq;
            }
        }
    }
    minDistance = std::sqrt(min_dSq);
}

void MetricsCalculator::calculateAllMetrics() {
    calculateEnergy(0);
    calculateMetricsFirstprivate();
    calculateFinalStateLastprivate();
}

void MetricsCalculator::saveMetricsToFile(const std::string& filename, double time) {
    std::ofstream out(filename, std::ios_base::app);
    if (out.is_open()) {
        out << std::fixed << std::setprecision(6)
            << time << " " 
            << kineticEnergy << " " 
            << potentialEnergy << " " 
            << totalEnergy << " " 
            << momentumMagnitude << " " 
            << cmX << " " 
            << cmY << " " 
            << rmsRadius << " " 
            << minDistance << "\n";
        out.close();
    } else {
        std::cerr << "Error al abrir archivo para escribir metricas: " << filename << "\n";
    }
}
