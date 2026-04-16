# Header-only library for graph representations.
#
# INTERFACE means: no compiled sources, just headers + compile settings.
# Targets that link against this get the include path automatically.

add_library(graph_lib INTERFACE)

target_include_directories(graph_lib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(graph_lib INTERFACE cxx_std_20)
