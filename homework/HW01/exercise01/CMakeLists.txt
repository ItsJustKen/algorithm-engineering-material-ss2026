cmake_minimum_required(VERSION 3.28...4.2)
project(ae2026_sheet1_1 CXX)
add_executable(output_id src/output_id.cpp)
target_compile_features(output_id PRIVATE cxx_std_20)
