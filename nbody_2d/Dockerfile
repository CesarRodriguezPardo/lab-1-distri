# =============================================================================
# Dockerfile — 2D N-Body Gravitational Simulator
# Laboratorio 1: Programación paralela con OpenMP
# Universidad de Santiago de Chile
#
# Provides a reproducible build + test environment with:
#   - g++ with OpenMP support (-fopenmp)
#   - make
#   - C++17 standard
#
# Build:
#   docker build -t nbody_2d .
#
# Run tests:
#   docker run --rm nbody_2d make test
#
# Run simulation (Week-1 verification):
#   docker run --rm nbody_2d ./nbody_2d
#
# Run benchmarks (Week 4):
#   docker run --rm nbody_2d make benchmark
#
# Interactive shell:
#   docker run --rm -it nbody_2d bash
# =============================================================================

FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# ---------------------------------------------------------------------------
# System dependencies
# ---------------------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
        g++ \
        make \
        libgomp1 \
        cmake \
        libgtest-dev \
        python3 \
        python3-pip \
        python3-matplotlib \
        python3-numpy \
    && rm -rf /var/lib/apt/lists/*

# Build and install GoogleTest from source (libgtest-dev ships source only on Ubuntu 22.04)
RUN cmake -S /usr/src/googletest -B /tmp/gtest_build \
    && cmake --build /tmp/gtest_build \
    && cmake --install /tmp/gtest_build \
    && rm -rf /tmp/gtest_build

# ---------------------------------------------------------------------------
# Working directory — mirrors the project layout inside the container
# ---------------------------------------------------------------------------
WORKDIR /app

# ---------------------------------------------------------------------------
# Copy source files
# We copy everything so Docker layer caching is still useful:
# changes to .cpp/.h files invalidate only from this layer onward.
# ---------------------------------------------------------------------------
COPY Makefile         ./
COPY *.h              ./
COPY *.cpp            ./
COPY tests/           ./tests/

# ---------------------------------------------------------------------------
# Build the main binary and the test runner
# Any compiler warning becomes visible here during the image build.
# ---------------------------------------------------------------------------
RUN make all
RUN make run_tests

# ---------------------------------------------------------------------------
# Default command: run the test suite
# Override with a different make target or binary at `docker run` time.
# ---------------------------------------------------------------------------
CMD ["make", "test"]
