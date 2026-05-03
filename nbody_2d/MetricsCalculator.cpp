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
        // Method 0: Using reduction
        #pragma omp parallel for reduction(+:K)
        for (int i = 0; i < n; ++i) {
            double vx = bodies[i].getVX();
            double vy = bodies[i].getVY();
            double v2 = vx * vx + vy * vy;
            K += 0.5 * bodies[i].getMass() * v2;
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
    } else if (method == 1) {
        // Method 1: Using atomic
        K = 0.0;
        U = 0.0;
        #pragma omp parallel for
        for (int i = 0; i < n; ++i) {
            double vx = bodies[i].getVX();
            double vy = bodies[i].getVY();
            double v2 = vx * vx + vy * vy;
            double k_local = 0.5 * bodies[i].getMass() * v2;
            
            #pragma omp atomic
            K += k_local;
        }

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < n; ++i) {
            double u_local = 0.0;
            for (int j = i + 1; j < n; ++j) {
                double dx = bodies[j].getX() - bodies[i].getX();
                double dy = bodies[j].getY() - bodies[i].getY();
                double distSq = dx * dx + dy * dy + eps * eps;
                u_local += (bodies[i].getMass() * bodies[j].getMass()) / std::sqrt(distSq);
            }
            double u_term = -G * u_local;
            
            #pragma omp atomic
            U += u_term;
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
    
    double last_processed_x = 0;
    double last_processed_y = 0;
    
    #pragma omp parallel for reduction(+:sumSqDist, totalMass) lastprivate(last_processed_x, last_processed_y)
    for(int i = 0; i < n; ++i) {
        double m = bodies[i].getMass();
        double dx = bodies[i].getX() - cmX;
        double dy = bodies[i].getY() - cmY;
        
        sumSqDist += m * (dx * dx + dy * dy);
        totalMass += m;
        
        // El último thread que ejecute la última iteración guardará estos valores
        last_processed_x = bodies[i].getX();
        last_processed_y = bodies[i].getY();
    }
    
    if (totalMass > 0) {
        rmsRadius = std::sqrt(sumSqDist / totalMass);
    }
    
    // Calculo de distancia minima
    double min_dSq = std::numeric_limits<double>::max();
    #pragma omp parallel for reduction(min:min_dSq) schedule(dynamic)
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
