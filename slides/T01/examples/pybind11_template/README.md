# pybind11 Template: Python ↔ C++ for Hot Loops

A complete, working example of moving a hot Python loop to C++ via pybind11 v3.
Uses TSP 2-opt as the example — same algorithm, same tour quality, ~50× speedup.

## Package structure

```
05_pybind11_template/
├── pyproject.toml              # Package metadata, version, build config
├── CMakeLists.txt              # C++ build rules (scikit-build-core)
├── src/fast_tsp/
│   ├── __init__.py             # Python package with __version__
│   ├── tsp.h                   # Pure C++ declarations (no pybind11 dependency)
│   ├── tsp.cpp                 # Pure C++ implementation
│   └── _bindings.cpp           # Thin pybind11 wrapper (Python pairs → std::vector<Point>)
├── tests/
│   ├── test_version.py         # Version consistency across Python/C++/pyproject
│   └── test_correctness.py     # Basic correctness tests
└── benchmark_python_vs_cpp.py  # Side-by-side comparison
```

## Setup and run

```bash
pip install -e ".[test]"          # build C++ extension + install
python benchmark_python_vs_cpp.py # compare Python vs C++
pytest tests/ -v                  # run tests
```

## Key design decisions

1. **Implementation separate from bindings.** `tsp.h`/`tsp.cpp` are pure C++ with
   no pybind11 dependency. `_bindings.cpp` is a thin wrapper that converts
   Python coordinate pairs into `std::vector<Point>`. The algorithms can be tested and reused from
   plain C++, and changing the Python interface doesn't recompile them.

2. **The entire loop is in C++** — not just `distance()`. Moving only the helper
   and calling it from Python would save function call overhead but not the
   Python loop overhead. You'd get ~2× instead of ~50×.

3. **Copy once, compute a lot.** The binding copies Python `(x, y)` pairs into
   `std::vector<Point>` once per call. For nearest-neighbor and 2-opt, that
   O(N) boundary cost is tiny compared with the O(N²) work in C++.

4. **`std::vector<int>` for small data** — pybind11 copies between Python lists
   and `std::vector`, but the tour is only N ints so the cost is negligible.

## Writing pybind11 bindings

### Minimal binding file

```cpp
#include <pybind11/pybind11.h>
namespace py = pybind11;

int add(int a, int b) { return a + b; }

PYBIND11_MODULE(_native, m) {
    m.doc() = "my extension module";
    m.def("add", &add, "Add two numbers", py::arg("a"), py::arg("b"));
}
```

### Wrapping functions that take Python coordinate pairs

```cpp
#include <array>
#include <pybind11/stl.h>

struct Point { double x, double y; };

static std::vector<Point> make_points(const std::vector<std::array<double, 2>>& cities) {
    std::vector<Point> points;
    points.reserve(cities.size());
    for (const auto& city : cities) {
        points.push_back(Point{city[0], city[1]});
    }
    return points;
}

m.def("process", [](const std::vector<std::array<double, 2>>& cities) {
    auto points = make_points(cities);
    return my_cpp_function(points);      // pure C++, no pybind11 dependency
}, py::arg("cities"));
```

Key points:
- The lambda keeps the binding thin; the real work is in pure C++
- The one-time O(N) copy is often irrelevant when the hot loop is O(N²)
- A named `Point` type keeps the C++ side easier to read than raw buffers

### STL containers and named arguments

```cpp
#include <pybind11/stl.h>  // automatic list ↔ vector conversion

// Python list ↔ std::vector (copied both ways — fine for small data)
m.def("sort_values", [](std::vector<int> v) {
    std::sort(v.begin(), v.end());
    return v;
}, py::arg("values"));
```

Named arguments with defaults:

```cpp
m.def("solve", &solve,
      "Run the solver on the given instance",
      py::arg("cities"),
      py::arg("timeout") = 10.0,         // default value
      py::arg("verbose") = false);
```

Enables `solve(cities, timeout=30)` on the Python side.

### Binding classes

Expose a C++ class so Python can construct it once and call methods without
repeatedly crossing the boundary. Useful when the object holds precomputed
state (e.g., a graph, a solver, a spatial index).

```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;

// Pure C++ class — no pybind11 dependency
struct DistanceMatrix {
    std::vector<double> data;
    int n;
    explicit DistanceMatrix(std::span<const Point> cities);
    double query(int i, int j) const { return data[i * n + j]; }
};

PYBIND11_MODULE(_native, m) {
    py::class_<DistanceMatrix>(m, "DistanceMatrix")
        .def(py::init([](const std::vector<std::array<double, 2>>& cities) {
            auto points = make_points(cities);
            return DistanceMatrix(points);
        }), py::arg("cities"))
        // Methods
        .def("query", &DistanceMatrix::query,
             py::arg("i"), py::arg("j"))
        // Read-only property
        .def_property_readonly("size", [](const DistanceMatrix& d) {
            return d.n;
        });
}
```

From Python: `dm = DistanceMatrix(cities)` builds once, then `dm.query(0, 5)`
is fast — the C++ object retains precomputed state across calls, so each method
invocation pays only the ~1–5 μs pybind11 overhead.

## scikit-build-core setup

### pyproject.toml (minimal)

```toml
[build-system]
requires = ["scikit-build-core>=0.12.2", "pybind11>=3.0.3"]
build-backend = "scikit_build_core.build"

[project]
name = "my-package"
version = "0.1.0"
requires-python = ">=3.10"

[tool.scikit-build]
minimum-version = "build-system.requires"
wheel.packages = ["src/my_package"]

[tool.scikit-build.cmake]
build-type = "Release"
```

### CMakeLists.txt (minimal)

```cmake
cmake_minimum_required(VERSION 3.15...3.30)
project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 3.0 CONFIG REQUIRED)

pybind11_add_module(_native src/my_package/_bindings.cpp src/my_package/impl.cpp)
target_compile_features(_native PRIVATE cxx_std_17)

install(TARGETS _native DESTINATION my_package)
```

Notes:
- `PYBIND11_FINDPYTHON ON` — uses modern FindPython, required for v3
- `build-type = "Release"` in pyproject.toml ensures the extension is compiled
  with optimization flags (`-O3` on GCC/Clang, `/O2` on MSVC) — without it,
  the default may be `-O0` and the extension would be barely faster than Python
- pybind11 does not support the Limited API (abi3) — expect per-CPython-minor wheels
- Version lives in three places (`pyproject.toml`, `__init__.py`, `_bindings.cpp`);
  `tests/test_version.py` checks all three agree to catch stale builds

## Guidelines for the Python↔C++ boundary

- **Every call has overhead** (~1–5 μs). Move the loop into C++, not just the helper.
- **Bind stateful objects** when the C++ side holds precomputed data across calls.
- **Prefer the simplest boundary that fits the asymptotics.** For O(N²) work, a one-time O(N) copy is often fine.
- **Use explicit C++ data types** (`Point`, `DistanceMatrix`) to keep the optimized side readable.
- **Compile with optimizations** (Release mode), never `-O0`.
- **Release the GIL** (`py::gil_scoped_release`) before pure C++ computations that take more than a few milliseconds, so other Python threads aren't blocked.
