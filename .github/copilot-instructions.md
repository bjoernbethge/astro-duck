# Copilot Instructions for astro-duck

## Repository Overview

**astro-duck** is a DuckDB extension providing comprehensive astronomical calculations and coordinate transformations. It includes functions for angular separation, coordinate conversion (RA/Dec ↔ Cartesian), photometric conversions, cosmological calculations, and integrations with Arrow, Spatial, and Parquet formats.

**Key Stats:**
- Language: C++ (C++17 required)
- Project Type: DuckDB Extension
- Size: Small (~10 source files + submodules)
- Target Runtime: DuckDB v1.4.3 (configurable via extension_config.cmake)
- Frameworks: CMake build system, DuckDB extension API

## Critical Build Requirements

### Prerequisites (MUST be installed)
- CMake 3.15+
- C++17 compatible compiler (GCC 13.3.0+ or equivalent)
- Python 3.7+ (for testing)
- Git (for submodule management)

### ALWAYS Initialize Submodules First

**CRITICAL**: Before ANY build command, ALWAYS run:
```bash
git submodule update --init --recursive
```

This initializes two required submodules:
- `duckdb/`: The DuckDB source code (required for building)
- `extension-ci-tools/`: Build system makefiles (required by Makefile)

**Failure to initialize submodules will cause immediate build failure** with error: `extension-ci-tools/makefiles/duckdb_extension.Makefile: No such file or directory`

### Build Commands

**Release Build** (recommended for testing extension functionality):
```bash
make clean && make release
```

**Debug Build** (for development with debug symbols):
```bash
make debug
```

**Build Time**: Initial release builds take 10-15+ minutes due to compiling DuckDB from source. Incremental builds are much faster (< 1 minute for extension-only changes).

**Build Output Locations:**
- Extension binary: `build/release/extension/astro/astro.duckdb_extension`
- DuckDB binary: `build/release/duckdb`

### Testing

**Two types of tests are available:**

1. **SQLLogicTests** (preferred by DuckDB, comprehensive):
```bash
make test          # Run all SQLLogicTests with release build
make test_debug    # Run all SQLLogicTests with debug build
```
   - Located in `test/sql/astro.test`
   - Contains 50 comprehensive tests covering all functions, edge cases, and NULL handling
   - Tests use DuckDB's standard SQLLogicTest format

2. **Python tests** (quick validation):
```bash
python test_astro.py    # Comprehensive test suite
python simple_test.py   # Quick smoke test
```
   - Tests require the extension to be built first
   - Tests use `build/release/duckdb` binary with `-unsigned` flag to load local extension
   - Tests verify all 8 astronomical functions plus integration features

**Test Requirements:**
- Tests require the extension to be built first
- SQLLogicTests use `require astro` directive to ensure extension is loaded
- Python tests manually load extension from `build/release/extension/astro/astro.duckdb_extension`

## Project Structure

### Root Directory Files
- `Makefile`: Main build orchestrator (includes extension-ci-tools makefiles)
- `CMakeLists.txt`: Extension-specific CMake configuration
- `extension_config.cmake`: Defines extension version (v1.0.0), DuckDB version (v1.4.0), and build configuration
- `test_astro.py`: Comprehensive Python test suite for all functions
- `simple_test.py`: Quick smoke test
- `.gitmodules`: Submodule configuration (duckdb, extension-ci-tools)
- `.gitignore`: Excludes build/, duckdb/, *.duckdb_extension, Python cache

### Source Code (`src/`)
- `src/astro.cpp`: Main implementation (~800 lines) containing all astronomical functions
- `src/include/astro.hpp`: Function declarations and extension header
- `src/include/astro_extension.hpp`: DuckDB extension interface

### Tests (`test/`)
- `test/sql/astro.test`: 50 comprehensive SQLLogicTests (DuckDB standard format)
  - Tests all functions, edge cases, NULL handling, batch processing
  - Uses `require astro` directive to ensure extension is loaded
- `test/README.md`: Testing documentation

### Documentation (`docs/`)
- `docs/UPDATING.md`: Guide for updating DuckDB version and handling API changes
- `docs/README.md`: Additional documentation

### Key Functions Implemented
1. `angular_separation(ra1, dec1, ra2, dec2)` - Haversine formula for angular distance
2. `radec_to_cartesian(ra, dec, distance)` - Coordinate conversion with JSON metadata
3. `mag_to_flux(magnitude, zero_point)` - Photometric conversion
4. `distance_modulus(distance_pc)` - Distance modulus calculation
5. `luminosity_distance(redshift, h0)` - Cosmological distance
6. `redshift_to_age(redshift)` - Universe age calculation
7. `celestial_point(ra, dec, distance)` - WKT geometry creation
8. `catalog_info(catalog_name)` - Catalog metadata (JSON)

### Configuration Files
- `.clang-format`, `.clang-tidy`, `.editorconfig`: All symlinked to `duckdb/` versions (follow DuckDB style)
- `.gitignore`: Excludes `build/`, `duckdb/`, `*.duckdb_extension`, Python cache

### CI/CD Workflows (`.github/workflows/`)
- `MainDistributionPipeline.yml`: Main CI using duckdb/extension-ci-tools (builds for all platforms)
- `ExtensionTemplate.yml`: Template testing workflow (can be ignored for astro extension)
- `deploy.yml`: Multi-platform deployment workflow (GitHub/S3/Community)

**CI Build Configuration:**
- Uses `duckdb_version: v1.4-andium` (not v1.4.3 as in extension_config.cmake)
- Excludes certain architectures on PRs to speed up CI
- Requires `-unsigned` flag when loading extension locally

## Common Issues and Workarounds

### Issue: Submodules Not Initialized
**Symptom:** `Makefile:8: extension-ci-tools/makefiles/duckdb_extension.Makefile: No such file or directory`
**Solution:** Run `git submodule update --init --recursive`

### Issue: Build Takes Very Long
**Expected:** First build takes 10-15+ minutes
**Workaround:** This is normal - DuckDB is being compiled from source. Be patient or use `make debug` for slightly faster builds.

### Issue: Test Failures with "Extension not found"
**Symptom:** Tests report extension binary not found
**Solution:** Ensure `build/release/extension/astro/astro.duckdb_extension` exists after successful build

### Issue: CMake Warning About vcpkg.json
**Symptom:** `Extension 'astro' has a vcpkg.json, but build was not run with VCPKG`
**Impact:** Warning can be ignored - vcpkg is optional for this extension
**Solution:** No action needed unless build fails

### Issue: CMake Deprecation Warning
**Symptom:** `Compatibility with CMake < 3.10 will be removed`
**Impact:** Cosmetic warning only - CMakeLists.txt uses `cmake_minimum_required(VERSION 3.5)`
**Solution:** Can be ignored or update to `cmake_minimum_required(VERSION 3.10)` in CMakeLists.txt

## Development Workflow

### Making Code Changes
1. **Always** initialize submodules first (if not done): `git submodule update --init --recursive`
2. Edit files in `src/astro.cpp` or headers
3. Build: `make clean && make release` (full rebuild recommended)
4. Test: `python test_astro.py`
5. For incremental changes, `make release` without `clean` is faster

### Adding New Functions
1. Declare in `src/include/astro.hpp`
2. Implement in `src/astro.cpp`
3. Register in extension's `LoadInternal()` function using `CreateScalarFunction()`
4. Add tests to `test/sql/astro.test` (SQLLogicTest format - preferred)
5. Optionally add Python tests to `test_astro.py`
6. Update `README.md` function table

### Clean Build
```bash
make clean && make release
```
Removes `build/` directory and rebuilds from scratch.

## Validation Pipeline

Before submitting PRs, the following must pass:

1. **Build:** `make release` (or `make debug`)
2. **Tests:** 
   - `make test` (SQLLogicTests - preferred, comprehensive)
   - OR `python test_astro.py` (Python tests)
3. **CI:** GitHub Actions runs `MainDistributionPipeline.yml`
   - Builds for linux_amd64, linux_arm64, osx_amd64, osx_arm64, windows_amd64, wasm_mvp
   - On PRs, most architectures are excluded for speed (only linux_amd64 is built)
   - Runs SQLLogicTests automatically

## Performance Characteristics

- Designed for vectorized batch processing
- Benchmarked at **10,000 objects in 0.003 seconds**
- Functions handle NULL inputs gracefully (return NULL)
- No external dependencies beyond DuckDB

## Quick Reference Commands

```bash
# Initial setup (REQUIRED)
git submodule update --init --recursive

# Build
make release                    # Release build (10-15 min first time)
make debug                     # Debug build
make clean && make release     # Clean rebuild

# Test
make test                      # SQLLogicTests (preferred)
make test_debug                # SQLLogicTests with debug build
python test_astro.py          # Python test suite
python simple_test.py         # Quick smoke test

# Check build output
ls -la build/release/extension/astro/
ls -la build/release/duckdb

# Load extension in DuckDB CLI
./build/release/duckdb -unsigned -c "LOAD 'build/release/extension/astro/astro.duckdb_extension'; SELECT angular_separation(0,0,1,1);"
```

## Important Notes

- **NEVER** modify files in `duckdb/` or `extension-ci-tools/` (submodules)
- **ALWAYS** use `-unsigned` flag when loading locally built extension
- **DO NOT** commit `build/` directory or `*.duckdb_extension` files (in .gitignore)
- Follow DuckDB's C++ style (enforced by .clang-format symlink)
- Extension version is defined in `extension_config.cmake`, not CMakeLists.txt
- Python tests expect specific JSON output format from functions (includes metadata like coordinate_system, epoch)
- When updating DuckDB version: see `docs/UPDATING.md` for proper procedure
  - Update duckdb submodule to latest tagged release
  - Update extension-ci-tools to matching branch (e.g., v1.1.0 branch for DuckDB v1.1.0)
  - Update duckdb_version in `.github/workflows/MainDistributionPipeline.yml`
  - Be prepared to handle C++ API changes (DuckDB's internal API is not stable)

## Trust These Instructions

These instructions are validated against the actual repository structure and build process. Follow them exactly to avoid common pitfalls. If you encounter issues not documented here, investigate before searching extensively - the problem is likely a missed step (usually submodule initialization).
