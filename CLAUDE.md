# Astro DuckDB Extension

DuckDB extension for astronomical calculations (coordinate transforms, angular separation, photometry, cosmology).

## Commands

```bash
# Build
make release              # Release build (outputs to build/release/)
make debug                # Debug build
make clean                # Clean build artifacts

# Test
python test_astro.py      # Run Python test suite
```

## Architecture

```
src/
  astro.cpp               # All function implementations
  include/
    astro.hpp             # Function declarations
    astro_extension.hpp   # Extension registration
test/
  sql/astro.test          # SQL test cases
test_astro.py             # Python integration tests
extension-ci-tools/       # DuckDB CI tooling (submodule)
duckdb/                   # DuckDB source (submodule)
```

## Key Files

- `extension_config.cmake` - Extension configuration for DuckDB build system
- `CMakeLists.txt` - CMake configuration
- `.github/workflows/MainDistributionPipeline.yml` - Main CI workflow

## Build Output

Extension binary: `build/release/extension/astro/astro.duckdb_extension`

## Testing Gotchas

- Tests use `duckdb -unsigned` flag to load unsigned extensions
- Extension must be loaded with full absolute path in tests
- DuckDB binary location: `./build/release/duckdb`

## Adding New Functions

1. Declare in `src/include/astro.hpp`
2. Implement in `src/astro.cpp`
3. Register in `LoadInternal()` using `CreateScalarFunction`
4. Add tests to `test_astro.py`
5. Update README.md function table

## CI

- Uses DuckDB extension-ci-tools workflow
- Builds for: linux_amd64, linux_arm64, osx_amd64, osx_arm64, windows_amd64, wasm_mvp
- Deploy triggered by version tags (v*)
