#pragma once

#include "NBodySystem.h"

/**
 * @brief Orchestrates the N-body simulation time loop.
 *
 * NBodySimulator drives the temporal evolution of an NBodySystem by
 * repeatedly calling force computation and integration steps. It also
 * houses the OpenMP variants required by the lab (Weeks 2-3):
 *
 *  - integrateEuler()                       — serial baseline
 *  - integrateEuler(int sync_type)          — atomic / critical / nowait
 *  - integrateEuler(int sync_type, bool)    — with explicit barrier control
 *  - calculateEnergy()                      — serial energy computation
 *  - calculateEnergy(int method)            — reduce / atomic variants
 *  - calculateEnergy(int method, bool)      — with private accumulators
 *  - processBodies()                        — serial body processing
 *  - processBodies(int task_type)           — task / parallel-for variants
 *  - processBodies(int task_type, bool)     — with single-thread init
 *  - simulatePhasesBarrier()               — explicit barrier between phases
 *  - parallelInitializationSingle()        — single-thread initialisation
 *  - calculateMetricsFirstprivate()        — firstprivate accumulators
 *  - calculateFinalStateLastprivate()      — lastprivate surviving values
 *
 * Week 1: only the constructor and run() are implemented. All OpenMP
 * overloads are declared here so the interface is stable; their bodies
 * are filled in during Weeks 2 and 3.
 */
class NBodySimulator {
private:
    NBodySystem* system;   ///< Non-owning pointer to the particle system
    double       time_step; ///< Δt for the integrator
    int          num_steps; ///< Total number of time steps to simulate

public:
    /**
     * @param sys   Pointer to the NBodySystem to simulate (must outlive this object)
     * @param dt    Time step Δt (should be small relative to orbital time-scales)
     * @param steps Number of integration steps (default 1 000)
     */
    NBodySimulator(NBodySystem* sys, double dt, int steps = 1000);

    // -----------------------------------------------------------------------
    // Week 1 — basic serial simulation loop
    // -----------------------------------------------------------------------

    /**
     * Runs the simulation for num_steps steps using the serial
     * computeAccelerations() and the Euler integrator.
     * Order per step: computeAccelerations → kick → drift.
     */
    void run();

    // -----------------------------------------------------------------------
    // Week 2-3 — integrateEuler overloads (OpenMP synchronisation variants)
    // sync_type: 0 = atomic, 1 = critical, 2 = nowait
    // -----------------------------------------------------------------------
    void integrateEuler();
    void integrateEuler(int sync_type);
    void integrateEuler(int sync_type, bool use_barrier);

    // -----------------------------------------------------------------------
    // Week 3 — energy / metric reduction overloads
    // method: 0 = reduce, 1 = atomic
    // -----------------------------------------------------------------------
    void calculateEnergy();
    void calculateEnergy(int method);
    void calculateEnergy(int method, bool use_private);

    // -----------------------------------------------------------------------
    // Week 3 — body-processing / task overloads
    // task_type: 0 = task, 1 = parallel_for
    // -----------------------------------------------------------------------
    void processBodies();
    void processBodies(int task_type);
    void processBodies(int task_type, bool use_single);

    // -----------------------------------------------------------------------
    // Week 3 — dedicated clause demonstrations
    // -----------------------------------------------------------------------
    void simulatePhasesBarrier();
    void parallelInitializationSingle();
    void calculateMetricsFirstprivate();
    void calculateFinalStateLastprivate();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------
    double getTimeStep() const;
    int    getNumSteps() const;
    double getCurrentTime() const;

private:
    double current_time; ///< Elapsed simulation time (incremented each step)
};