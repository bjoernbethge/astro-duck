# Code Style and Conventions

## C++ Style
- Uses DuckDB's `.clang-format` (references `duckdb/.clang-format`)
- Tab indentation
- Namespace: `duckdb`
- Header guards: `#pragma once`

## Naming Conventions
- Classes: PascalCase (e.g., `AstroExtension`, `AstroGeometryProcessor`)
- Functions: PascalCase for public functions (e.g., `RADecToCartesian`, `AngularSeparation`)
- Constants: UPPER_SNAKE_CASE (e.g., `SPEED_OF_LIGHT`, `M_PI`)
- Variables: snake_case (e.g., `ra_deg`, `dec_rad`)

## DuckDB Extension Patterns
- Define `DUCKDB_EXTENSION_MAIN` before includes
- Use `make_uniq<T>()` for unique pointers
- Register functions via `CreateScalarFunction` in `LoadInternal()`
- Extension class inherits from `Extension`
- Override `Load()`, `Name()`, `Version()`

## Function Registration Pattern
```cpp
CreateScalarFunction("function_name", 
    {LogicalType::DOUBLE, LogicalType::DOUBLE},  // input types
    LogicalType::DOUBLE,                          // return type
    FunctionImplementation);
```

## Comments
- Use `// ===== SECTION NAME =====` for major sections
- Brief inline comments for complex calculations
- No docstrings (C++ style)

## Windows Compatibility
- Define `M_PI` if not available:
```cpp
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
```
