# CMake build rules for the pybind11 C++ extension.
#
# scikit-build-core invokes this automatically during `pip install -e .`.
# You never need to run cmake manually.

# 3.15+ required by scikit-build-core for proper Python discovery.
# Upper bound documents the latest version tested against.
cmake_minimum_required(VERSION 3.15...3.30)

# SKBUILD_PROJECT_NAME is set by scikit-build-core from pyproject.toml [project].name.
# Using it avoids duplicating the project name between pyproject.toml and CMakeLists.txt.
# LANGUAGES CXX tells CMake we only need a C++ compiler (skips checking for C, Fortran, etc.).
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

# Use CMake's modern FindPython (not the legacy FindPythonInterp/FindPythonLibs).
# pybind11 v3 defaults to this, but setting it explicitly ensures consistent behavior
# even if an older pybind11 config is found on the system.
set(PYBIND11_FINDPYTHON ON)

# Locate pybind11's CMake config. scikit-build-core ensures pybind11
# is installed (via pyproject.toml build-requires) before this runs.
# The version constraint ensures we get the pybind11 v3 API.
find_package(pybind11 3.0 CONFIG REQUIRED)

# Build the C++ sources as a Python extension module named "_native".
# The leading underscore is a convention indicating "private C++ implementation";
# users import from the public fast_tsp package, not from _native directly.
# pybind11_add_module handles the platform-specific shared library suffix
# (.so on Linux, .pyd on Windows, .dylib on macOS).
#
# Two source files:
# - tsp.cpp: pure C++ implementation (no pybind11 dependency)
# - _bindings.cpp: thin pybind11 wrapper that converts Python pairs → std::vector<Point>
pybind11_add_module(_native
    src/fast_tsp/tsp.cpp
    src/fast_tsp/_bindings.cpp
)

# Require C++20. We use std::span in the pure C++ layer for read-only views
# over Point collections and DistanceMatrix input. pybind11 v3 needs at least C++17.
# PRIVATE means this requirement only applies to building _native,
# not to anything that links against it.
target_compile_features(_native PRIVATE cxx_std_20)

# Install the compiled extension into the fast_tsp package directory.
# scikit-build-core maps this to the correct site-packages location.
# The DESTINATION must match the Python package name so that
# `from fast_tsp._native import ...` works after installation.
install(TARGETS _native DESTINATION fast_tsp)
