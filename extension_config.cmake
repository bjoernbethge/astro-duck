# This file is included by DuckDB's build system. It specifies which extension to load
set(EXTENSION_VERSION "v1.1.0")
set(DUCKDB_VERSION "v1.4.3")
set(GIT_COMMIT_HASH "v1.4.3")
# Astro Extension - Astronomical calculations and coordinate transformations
duckdb_extension_load(astro
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}
    INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/src/include
    SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/astro.cpp
)

# Any extra extensions that should be built
# e.g.: duckdb_extension_load(json)