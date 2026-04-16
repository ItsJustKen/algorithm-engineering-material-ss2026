# Build unit tests using Google Test.
#
# Run after building:  ./build/tests/test_graph
# Or via ctest:        cd build && ctest

add_executable(test_graph test_graph.cpp)

# Link against our graph library and Google Test.
# graph_lib provides the include path and C++20 requirement automatically.
# gtest_main provides main() so we don't need to write our own.
target_link_libraries(test_graph graph_lib gtest_main)

# Register with ctest so `ctest --test-dir build` works.
include(GoogleTest)
gtest_discover_tests(test_graph)
