"""
Test that version strings are consistent across pyproject.toml, Python, and C++.

A mismatch means either:
- The version wasn't updated in all three places, or
- The C++ extension is stale (needs recompile: pip install -e .)

Run:
    pytest tests/
"""

import importlib.metadata

import fast_tsp
from fast_tsp._native import __version__ as native_version


def _get_pyproject_version() -> str:
    """Get the installed package version (from pyproject.toml metadata)."""
    return importlib.metadata.version("fast-tsp")


def test_python_matches_pyproject():
    """Python __version__ must match pyproject.toml version."""
    pyproject_version = _get_pyproject_version()
    assert fast_tsp.__version__ == pyproject_version, (
        f"Python __version__ ({fast_tsp.__version__!r}) does not match "
        f"pyproject.toml version ({pyproject_version!r}). "
        f"Update __version__ in src/fast_tsp/__init__.py."
    )


def test_native_matches_python():
    """C++ __version__ must match Python __version__.

    This is the most likely failure: you bumped the Python version
    but forgot to recompile the C++ extension.
    """
    assert native_version == fast_tsp.__version__, (
        f"C++ native __version__ ({native_version!r}) does not match "
        f"Python __version__ ({fast_tsp.__version__!r}). "
        f"The C++ extension is probably stale. Recompile with: pip install -e ."
    )
