#!/bin/bash
#SBATCH --job-name=nbody-bench-suite
#SBATCH --partition=GPU
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --gres=gpu:A30:1
#SBATCH --time=00:10:00
#SBATCH --output=results/logs/slurm-%A_%a.out
#SBATCH --error=results/logs/slurm-%A_%a.err
#SBATCH --array=1-10%2

# Entorno CUDA (cuda-12.1; /usr/local/cuda apunta a 10.2 - NO usar)
export PATH=/usr/local/cuda-12.1/bin:$PATH
export LD_LIBRARY_PATH=/usr/local/cuda-12.1/lib64:$LD_LIBRARY_PATH

SUBMIT=$SLURM_SUBMIT_DIR
REP=$SLURM_ARRAY_TASK_ID
mkdir -p "$SUBMIT/results/raw" "$SUBMIT/results/logs"
WORKDIR="$SUBMIT/results/work/rep${REP}"
mkdir -p "$WORKDIR"

LOG="$SUBMIT/results/logs/cluster_run_rep${REP}.log"
{
  echo "=== CLUSTER RUN LOG ==="
  echo "JobID: $SLURM_JOB_ID"
  echo "ArrayJobID: $SLURM_ARRAY_JOB_ID"
  echo "ArrayTaskID: $REP"
  echo "Node: $(hostname)"
  echo "GPU: $(nvidia-smi --query-gpu=name,driver_version,memory.total --format=csv,noheader)"
  echo "CUDA: $(nvcc --version | grep release)"
  echo "Flags: nvcc -O3 -std=c++17 -I. -Ikernels -Xcompiler -Wall,-Wextra,-fopenmp"
  echo "Matrix: N={256,512,1024,2000} x variants={0,1} x blockDim={64,128,256,512,1024} x {kernel-only,end-to-end} (10 reps internas)"
  echo "Date: $(date)"
  echo "======================="
} > "$LOG"

# Workdir unico por replica (evita colision NFS en benchmark_results.dat)
cd "$WORKDIR"
printf '42\n0\n256\n0.01\n1.0\n0.01\n100\n3\n' | "$SUBMIT/nbody" >> "$LOG" 2>&1
RC=$?

if [ $RC -eq 0 ] && [ -f benchmark_results.dat ]; then
  mv benchmark_results.dat "$SUBMIT/results/raw/benchmark_results_rep${REP}.dat"
  echo "ALL BENCHMARKS COMPLETED rc=$RC: $(date)" >> "$LOG"
else
  echo "[ERROR] rc=$RC o .dat faltante: $(date)" >> "$LOG"
  exit 1
fi
