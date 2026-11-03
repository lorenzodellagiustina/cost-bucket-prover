# Development container for the cost-counter pass: LLVM 14, the version of the SeaHorn image, built
# for the host's architecture (no emulation on Apple Silicon). The SeaHorn image itself (amd64) is
# built and run through Docker outside of Docker, for the end-to-end tests.
FROM mcr.microsoft.com/devcontainers/base:ubuntu-22.04

# Install dependencies.
# Command structured following docker suggestion: https://docs.docker.com/develop/develop-images/dockerfile_best-practices/#run.
# libffi, libtinfo, zlib, libxml2 and libedit are required by LLVM 14's CMake package.
RUN apt-get update && apt-get install -y \
    clang-14 \
    clangd-14 \
    cmake \
    g++ \
    libedit-dev \
    libffi-dev \
    libtinfo-dev \
    libxml2-dev \
    llvm-14 \
    llvm-14-dev \
    make \
    ninja-build \
    python3 \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*
