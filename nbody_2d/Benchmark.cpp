#include "Benchmark.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <numeric>
#include <stdexcept>

// =============================================================================
// Constructor
// =============================================================================

Benchmark::Benchmark(NBodySystem* sys, int repetitions)
    : system(sys), repetitions(repetitions), baseline_time(0.0)
{
    if (!sys)
        throw std::invalid_argument("Benchmark: system pointer must not be null");
    if (repetitions < 1)
        throw std::invalid_argument("Benchmark: repetitions must be >= 1");
}

// =============================================================================
// Week 4 — public benchmark runners (stubs)
// =============================================================================

/**
 * TODO (Week 4): Benchmark computeAccelerations() with static, dynamic, and
 * guided schedules for a range of chunk sizes. For each combination, time
 * `repetitions` calls with omp_get_wtime(), compute mean and stddev, and
 * append a row to the results table. Write output to "benchmark_results.dat".
 */
void Benchmark::runScheduleBenchmarks()
{
    std::cout << "[Benchmark] runScheduleBenchmarks() — not yet implemented (Week 4)\n";
}

/**
 * TODO (Week 4): Benchmark the synchronisation strategies used in energy and
 * metrics computation (atomic, critical, reduce). Write output to
 * "benchmark_results.dat".
 */
void Benchmark::runSyncBenchmarks()
{
    std::cout << "[Benchmark] runSyncBenchmarks() — not yet implemented (Week 4)\n";
}

/**
 * TODO (Week 4): Sweep thread count 1 … max_threads, call
 * computeAccelerations() `repetitions` times per count, compute speedup S_p
 * and efficiency E_p with error propagation, and write to
 * "scaling_analysis.dat".
 */
void Benchmark::runScalingAnalysis(int max_threads)
{
    (void)max_threads;
    std::cout << "[Benchmark] runScalingAnalysis() — not yet implemented (Week 4)\n";
}

/**
 * TODO (Week 4): Instrument the full simulation step to measure the serial
 * fraction f (initialisation, I/O, metrics) vs. the parallel fraction
 * (computeAccelerations). Compare measured f with Amdahl's prediction.
 */
void Benchmark::measureSerialFraction()
{
    std::cout << "[Benchmark] measureSerialFraction() — not yet implemented (Week 4)\n";
}

/**
 * TODO (Week 4): Run all sub-benchmarks in sequence and produce the
 * required output files and PNG plots (via a Python script or Gnuplot).
 */
void Benchmark::runAll()
{
    std::cout << "[Benchmark] runAll() — not yet implemented (Week 4)\n";
    runScheduleBenchmarks();
    runSyncBenchmarks();
    runScalingAnalysis();
    measureSerialFraction();
}

// =============================================================================
// Accessors
// =============================================================================

double Benchmark::getBaselineTime() const
{
    return baseline_time;
}

const std::vector<std::vector<double>>& Benchmark::getResults() const
{
    return results;
}

// =============================================================================
// Private helpers
// =============================================================================

double Benchmark::mean(const std::vector<double>& samples)
{
    if (samples.empty()) return 0.0;
    double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    return sum / static_cast<double>(samples.size());
}

double Benchmark::stddev(const std::vector<double>& samples, double sample_mean)
{
    if (samples.size() < 2) return 0.0;
    double sq_sum = 0.0;
    for (double v : samples) {
        double diff = v - sample_mean;
        sq_sum += diff * diff;
    }
    return std::sqrt(sq_sum / static_cast<double>(samples.size()));
}

/**
 * Error propagation for speedup S_p = T_1 / T_p (PDF §5.3.1):
 *   sigma_Sp = S_p * sqrt( (sigma_T1 / T1)^2 + (sigma_Tp / Tp)^2 )
 */
double Benchmark::speedupError(double Sp,
                               double T1, double sigma_T1,
                               double Tp, double sigma_Tp)
{
    if (T1 <= 0.0 || Tp <= 0.0) return 0.0;
    double rel1 = sigma_T1 / T1;
    double relp = sigma_Tp / Tp;
    return Sp * std::sqrt(rel1 * rel1 + relp * relp);
}

/**
 * Error propagation for efficiency E_p = S_p / p (PDF §5.3.1):
 *   sigma_Ep = sigma_Sp / p
 */
double Benchmark::efficiencyError(double sigma_Sp, int p)
{
    if (p <= 0) return 0.0;
    return sigma_Sp / static_cast<double>(p);
}

/**
 * Writes the accumulated results table to a .dat file.
 * Columns: threads  T_mean  T_std  Speedup  Speedup_err  Efficiency  Efficiency_err
 */
void Benchmark::writeDAT(const std::string& filename) const
{
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "[Benchmark] Warning: could not open " << filename << " for writing\n";
        return;
    }
    out << "# threads  T_mean(s)  T_std(s)  Speedup  Sp_err  Efficiency  Ep_err\n";
    for (const auto& row : results) {
        for (size_t c = 0; c < row.size(); ++c) {
            out << row[c];
            if (c + 1 < row.size()) out << "  ";
        }
        out << "\n";
    }
    std::cout << "[Benchmark] Results written to " << filename << "\n";
}