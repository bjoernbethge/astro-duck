# Astro-Duck Project Overview

## Purpose
DuckDB extension providing astronomical calculations and coordinate transformations. Functions include:
- Angular separation (Haversine formula)
- RA/Dec to Cartesian coordinate conversion
- Magnitude/flux photometric conversions
- Cosmological calculations (luminosity distance, redshift to age)
- WKT geometry creation for spatial integration

## Tech Stack
- **Language**: C++17
- **Build System**: CMake 3.15+
- **Database**: DuckDB extension API (v1.3.0+)
- **CI/CD**: GitHub Actions with DuckDB extension-ci-tools
- **Testing**: Python test suite + SQL tests

## Target Platforms
- linux_amd64, linux_arm64
- osx_amd64, osx_arm64 (Apple Silicon)
- windows_amd64
- wasm_mvp (WebAssembly)

## Repository Structure
```
src/
  astro.cpp                 # All function implementations
  include/
    astro.hpp               # Extension class declaration
    astro_extension.hpp     # Extension registration
test/
  sql/astro.test            # SQL test cases
test_astro.py               # Python integration tests
extension-ci-tools/         # DuckDB CI tooling (submodule)
duckdb/                     # DuckDB source (submodule)
.github/workflows/          # CI/CD pipelines
```

## Key Files
- `extension_config.cmake` - Extension config for DuckDB build
- `CMakeLists.txt` - CMake configuration
- `Makefile` - Wraps extension-ci-tools makefile
