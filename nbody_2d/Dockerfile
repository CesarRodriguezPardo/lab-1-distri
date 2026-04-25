# Base image for the GitHub Actions workflow (and local parity): Debian + C++ build
# tools. The workflow mounts the repo at /workspace and runs make there;
# this file does not copy or build the project itself.
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    ca-certificates \
    g++ \
    make \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["bash"]