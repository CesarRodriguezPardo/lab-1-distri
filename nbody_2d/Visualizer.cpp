#include "Visualizer.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

// =============================================================================
// Constructor
// =============================================================================

Visualizer::Visualizer(const NBodySystem* sys,
                       const std::string& out_dir,
                       int                sample_every)
    : system(sys), output_dir(out_dir), sample_every(sample_every)
{
    if (!sys)
        throw std::invalid_argument("Visualizer: system pointer must not be null");
    if (sample_every <= 0)
        throw std::invalid_argument("Visualizer: sample_every must be > 0");
}

// =============================================================================
// Week 5 — Trajectory / snapshot recording (stubs)
// =============================================================================

/**
 * Records the current system state into the internal trajectory buffer.
 * Each snapshot stores: time, particle_id, x, y, vx, vy for every body.
 *
 * Week 5: full implementation pending.
 */
void Visualizer::recordSnapshot(double current_time)
{
    // TODO (Week 5): iterate over system->getBodies() and push
    // { current_time, id, x, y, vx, vy } rows into trajectory_buffer.
    (void)current_time;
}

/**
 * Writes all buffered snapshots to "<output_dir>/trajectories.dat".
 *
 * File format (one line per particle per snapshot):
 *   # time  particle_id  x  y  vx  vy
 *   <t>  <id>  <x>  <y>  <vx>  <vy>
 *
 * Week 5: full implementation pending.
 */
void Visualizer::writeTrajectories() const
{
    // TODO (Week 5): open output_dir + "/trajectories.dat" and write
    // trajectory_buffer contents row by row.
    std::cout << "[Visualizer] writeTrajectories(): not yet implemented (Week 5).\n";
}

/**
 * Writes a single full-system snapshot to "<output_dir>/snapshots.dat".
 *
 * File format:
 *   # Snapshot at t = <current_time>
 *   # particle_id  mass  x  y  vx  vy  ax  ay
 *   <id>  <m>  <x>  <y>  <vx>  <vy>  <ax>  <ay>
 *
 * Week 5: full implementation pending.
 */
void Visualizer::writeSnapshot(double current_time) const
{
    // TODO (Week 5): open output_dir + "/snapshots.dat" (append mode) and
    // write the current state of every body in system->getBodies().
    (void)current_time;
    std::cout << "[Visualizer] writeSnapshot(): not yet implemented (Week 5).\n";
}

/**
 * Writes K(t), U(t), E(t) time series to "<output_dir>/<filename>".
 *
 * File format:
 *   # time  K  U  E
 *   <t>  <K>  <U>  <E>
 *
 * Week 5: full implementation pending.
 */
void Visualizer::writeEnergyTimeSeries(const std::vector<double>& times,
                                       const std::vector<double>& K_vals,
                                       const std::vector<double>& U_vals,
                                       const std::string& filename) const
{
    // TODO (Week 5): validate vector sizes, open output_dir + "/" + filename,
    // write header and one row per time sample.
    (void)times; (void)K_vals; (void)U_vals; (void)filename;
    std::cout << "[Visualizer] writeEnergyTimeSeries(): not yet implemented (Week 5).\n";
}

/**
 * Clears the internal trajectory buffer.
 * Useful between benchmark runs or simulation segments.
 */
void Visualizer::clearBuffer()
{
    trajectory_buffer.clear();
}

// =============================================================================
// Accessors
// =============================================================================

int         Visualizer::getSampleEvery() const { return sample_every; }
std::string Visualizer::getOutputDir()   const { return output_dir;   }

void Visualizer::setSampleEvery(int n)
{
    if (n <= 0)
        throw std::invalid_argument("Visualizer::setSampleEvery: n must be > 0");
    sample_every = n;
}

void Visualizer::setOutputDir(const std::string& dir)
{
    output_dir = dir;
}