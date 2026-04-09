#pragma once

#include "NBodySystem.h"
#include <string>
#include <vector>

/**
 * @brief Handles trajectory export and snapshot generation for visualization.
 *
 * Exports simulation state to .dat files that can be plotted with
 * Python/Matplotlib or Gnuplot.
 *
 * Week 1 : Stub declaration — interfaces defined, implementations pending.
 * Week 5 : Full implementation of trajectory recording, snapshot export,
 *           and integration with the make test / CI pipeline.
 *
 * Output files (written to working directory):
 *   trajectories.dat  — sampled positions (and optionally velocities) over time
 *   snapshots.dat     — full system state at selected time steps
 */
class Visualizer {
private:
    const NBodySystem* system;   ///< Non-owning pointer to the simulated system
    std::string        output_dir;
    int                sample_every; ///< Record state every N steps

    /// Internal buffer: each entry is one sampled step
    std::vector<std::vector<double>> trajectory_buffer;

public:
    /**
     * @param sys         The N-body system to observe (non-owning).
     * @param out_dir     Directory where output files will be written (default ".").
     * @param sample_every Record a snapshot every this many steps (default 10).
     */
    explicit Visualizer(const NBodySystem* sys,
                        const std::string& out_dir    = ".",
                        int                sample_every = 10);

    // -----------------------------------------------------------------------
    // Week 5: trajectory / snapshot recording
    // -----------------------------------------------------------------------

    /**
     * Records the current system state into the internal buffer.
     * Call once per simulation step (or once every sample_every steps).
     * @param current_time Simulation time at this snapshot.
     */
    void recordSnapshot(double current_time);

    /**
     * Writes all buffered snapshots to trajectories.dat.
     * Format (one line per particle per snapshot):
     *   time  particle_id  x  y  vx  vy
     */
    void writeTrajectories() const;

    /**
     * Writes the current system state (single frame) to snapshots.dat.
     * Useful for saving the final state or periodic full dumps.
     * @param current_time Simulation time label written in the header comment.
     */
    void writeSnapshot(double current_time) const;

    /**
     * Writes the energy time series produced by an external source to a file.
     * @param times   Vector of simulation times.
     * @param K_vals  Kinetic energy at each time.
     * @param U_vals  Potential energy at each time.
     * @param filename Output filename (default "energy_timeseries.dat").
     */
    void writeEnergyTimeSeries(const std::vector<double>& times,
                               const std::vector<double>& K_vals,
                               const std::vector<double>& U_vals,
                               const std::string& filename = "energy_timeseries.dat") const;

    /**
     * Clears the internal trajectory buffer (e.g. between benchmark runs).
     */
    void clearBuffer();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------
    int         getSampleEvery() const;
    std::string getOutputDir()   const;
    void        setSampleEvery(int n);
    void        setOutputDir(const std::string& dir);
};