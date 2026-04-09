#pragma once

#include "NBodySystem.h"
#include <string>
#include <vector>

/**
 * @brief Benchmarking module for the N-body gravitational simulator.
 *
 * Measures and reports execution times for the different OpenMP variants of
 * computeAccelerations() and the full simulation step, sweeping over N and
 * thread counts.
 *
 * Timing is done with omp_get_wtime(). Each experiment is repeated at least
 * 10 times; mean and standard deviation are reported in the form T = T̄ ± σ_T.
 *
 * Metrics computed:
 *   - Speedup:    S_p = T_1 / T_p
 *   - Efficiency: E_p = S_p / p
 *   - Amdahl prediction: S_p = 1 / (f + (1-f)/p)
 *   - Error propagation on S_p and E_p (see PDF §5.3.1)
 *
 * Week 1: Stub — class declared, full implementation in Week 4.
 */
class Benchmark {
public:
    /**
     * @param sys          Pointer to the NBodySystem to benchmark (not owned).
     * @param repetitions  Number of timed repetitions per experiment (default 10).
     */
    explicit Benchmark(NBodySystem* sys, int repetitions = 10);

    // -----------------------------------------------------------------------
    // Week 4 — Schedule benchmarks
    // -----------------------------------------------------------------------
    /**
     * Benchmarks computeAccelerations with static, dynamic, and guided
     * schedules for various chunk sizes, writing results to
     * "benchmark_results.dat".
     */
    void runScheduleBenchmarks();

    /**
     * Benchmarks synchronization strategies (atomic, critical, reduce) used
     * in energy/metrics computation, writing results to
     * "benchmark_results.dat".
     */
    void runSyncBenchmarks();

    /**
     * Sweeps thread count (1 … max_threads) and computes speedup / efficiency
     * for computeAccelerations(), writing to "scaling_analysis.dat".
     * @param max_threads Upper bound on thread count (defaults to hardware).
     */
    void runScalingAnalysis(int max_threads = 0);

    /**
     * Measures the serial and parallel fractions of the full simulation step
     * to estimate f (Amdahl serial fraction) experimentally.
     */
    void measureSerialFraction();

    /**
     * Runs all benchmarks in sequence and saves summary files + PNG plots.
     */
    void runAll();

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------
    /** Returns the last recorded T_1 (single-thread time, seconds). */
    double getBaselineTime() const;

    /** Returns the results table as a vector of rows [threads, T_mean, T_std, Sp, Ep]. */
    const std::vector<std::vector<double>>& getResults() const;

private:
    NBodySystem* system;      ///< System under test (not owned)
    int repetitions;          ///< Repetitions per measurement
    double baseline_time;     ///< T_1: single-thread reference time (seconds)

    std::vector<std::vector<double>> results; ///< Accumulated result rows

    /** Computes mean of a sample. */
    static double mean(const std::vector<double>& samples);

    /** Computes population standard deviation of a sample. */
    static double stddev(const std::vector<double>& samples, double sample_mean);

    /**
     * Propagates error on speedup: σ_Sp = Sp * sqrt((σ_T1/T1)^2 + (σ_Tp/Tp)^2).
     */
    static double speedupError(double Sp,
                               double T1, double sigma_T1,
                               double Tp, double sigma_Tp);

    /**
     * Propagates error on efficiency: σ_Ep = σ_Sp / p.
     */
    static double efficiencyError(double sigma_Sp, int p);

    /** Writes results table to the given .dat file. */
    void writeDAT(const std::string& filename) const;
};