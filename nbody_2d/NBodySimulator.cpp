#include "NBodySimulator.h"
#include "Integrator.h"
#include <iostream>
#include <stdexcept>

// =============================================================================
// Constructor
// =============================================================================

NBodySimulator::NBodySimulator(NBodySystem* sys, double dt, int steps)
    : system(sys), time_step(dt), num_steps(steps), current_time(0.0)
{
    if (!sys)
        throw std::invalid_argument("NBodySimulator: system pointer must not be null");
    if (dt <= 0.0)
        throw std::invalid_argument("NBodySimulator: time_step must be > 0");
    if (steps <= 0)
        throw std::invalid_argument("NBodySimulator: num_steps must be > 0");
}

// =============================================================================
// Week 1 — Serial simulation loop
// =============================================================================

/**
 * Runs the simulation for num_steps steps using the serial Euler integrator.
 *
 * Each step follows the PDF-mandated order (§4.2.1):
 *   1. computeAccelerations() — all forces from current positions.
 *   2. kick() — all velocities updated: v_i <- v_i + a_i * dt.
 *   3. drift() — all positions updated: r_i <- r_i + v_i * dt.
 *
 * Week 2 will add OpenMP-parallelised overloads of computeAccelerations()
 * that this loop will be able to call by passing a schedule_type parameter.
 */
void NBodySimulator::run()
{
    for (int step = 0; step < num_steps; step++) {
        Integrator::eulerStep(*system, time_step);
        current_time += time_step;
    }
}

// =============================================================================
// Week 2-3 — integrateEuler overloads (OpenMP synchronisation variants)
// =============================================================================
// sync_type: 0 = atomic, 1 = critical, 2 = nowait
// These will be fully implemented in Week 2 when the parallel
// computeAccelerations() overloads are added to NBodySystem.
// =============================================================================

void NBodySimulator::integrateEuler()
{
    // TODO (Week 2): parallel computeAccelerations with default schedule
    // For now delegates to the serial baseline.
    Integrator::eulerStep(*system, time_step);
    current_time += time_step;
}

void NBodySimulator::integrateEuler(int sync_type)
{
    // TODO (Week 2): select atomic / critical / nowait path based on sync_type
    (void)sync_type;
    integrateEuler();
}

void NBodySimulator::integrateEuler(int sync_type, bool use_barrier)
{
    // TODO (Week 2): add explicit barrier between force and integration phases
    (void)use_barrier;
    integrateEuler(sync_type);
}

// =============================================================================
// Week 3 — calculateEnergy overloads (reduction strategy variants)
// =============================================================================
// method: 0 = OpenMP reduce clause, 1 = atomic accumulation
// =============================================================================

void NBodySimulator::calculateEnergy()
{
    // TODO (Week 3): serial K + U computation; delegate to MetricsCalculator
    std::cout << "[NBodySimulator::calculateEnergy] Not yet implemented (Week 3)\n";
}

void NBodySimulator::calculateEnergy(int method)
{
    // TODO (Week 3): parallel reduction via reduce (method=0) or atomic (method=1)
    (void)method;
    calculateEnergy();
}

void NBodySimulator::calculateEnergy(int method, bool use_private)
{
    // TODO (Week 3): thread-private accumulators before final critical reduction
    (void)use_private;
    calculateEnergy(method);
}

// =============================================================================
// Week 3 — processBodies overloads (task vs parallel_for)
// =============================================================================
// task_type: 0 = OpenMP task, 1 = parallel for
// =============================================================================

void NBodySimulator::processBodies()
{
    // TODO (Week 3): serial body-index processing baseline
    std::cout << "[NBodySimulator::processBodies] Not yet implemented (Week 3)\n";
}

void NBodySimulator::processBodies(int task_type)
{
    // TODO (Week 3): dispatch to task or parallel-for variant
    (void)task_type;
    processBodies();
}

void NBodySimulator::processBodies(int task_type, bool use_single)
{
    // TODO (Week 3): single-thread initialisation block before the parallel region
    (void)use_single;
    processBodies(task_type);
}

// =============================================================================
// Week 3 — Dedicated OpenMP clause demonstrations
// =============================================================================

void NBodySimulator::simulatePhasesBarrier()
{
    // TODO (Week 3): demonstrates explicit #pragma omp barrier between
    // force-computation and integration phases inside a single parallel region.
    std::cout << "[NBodySimulator::simulatePhasesBarrier] Not yet implemented (Week 3)\n";
}

void NBodySimulator::parallelInitializationSingle()
{
    // TODO (Week 3): demonstrates #pragma omp single for one-time initialisation
    // (e.g. seeding RNG or printing a header) inside a parallel region.
    std::cout << "[NBodySimulator::parallelInitializationSingle] Not yet implemented (Week 3)\n";
}

void NBodySimulator::calculateMetricsFirstprivate()
{
    // TODO (Week 3): illustrates firstprivate — each thread starts with a copy
    // of a pre-initialised accumulator rather than a default-constructed one.
    std::cout << "[NBodySimulator::calculateMetricsFirstprivate] Not yet implemented (Week 3)\n";
}

void NBodySimulator::calculateFinalStateLastprivate()
{
    // TODO (Week 3): illustrates lastprivate — the value from the thread that
    // executes the last iteration is written back to the shared variable.
    std::cout << "[NBodySimulator::calculateFinalStateLastprivate] Not yet implemented (Week 3)\n";
}

// =============================================================================
// Accessors
// =============================================================================

double NBodySimulator::getTimeStep()    const { return time_step;    }
int    NBodySimulator::getNumSteps()    const { return num_steps;     }
double NBodySimulator::getCurrentTime() const { return current_time;  }