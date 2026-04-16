cmake_minimum_required(VERSION 3.28...4.2)
project(tsp_two_opt CXX)

find_package(pybind11 REQUIRED)

pybind11_add_module(_core src/tsp_two_opt/_core.cpp)
target_compile_features(_core PRIVATE cxx_std_20)

install(TARGETS _core DESTINATION tsp_two_opt)
