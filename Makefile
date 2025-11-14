PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=astro
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Build-Parallelisierung (18 von 22 Kernen)
CORES ?= 18
MAKEFLAGS += -j$(CORES)

# Build-Optimierungen
export CMAKE_BUILD_PARALLEL_LEVEL=$(CORES)
export NINJA_STATUS="[%f/%t] "

# Compiler-Cache für schnellere Rebuilds (nur wenn ccache verfügbar ist)
ifneq ($(shell which ccache 2>/dev/null),)
export CCACHE_DIR=/tmp/ccache
export CCACHE_MAXSIZE=5G
export CC=ccache gcc
export CXX=ccache g++
else
export CC=gcc
export CXX=g++
endif

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# Detect OS and set executable extension
# Primary method: Check DUCKDB_PLATFORM (set by CI, e.g., windows_amd64)
# Fallback: Check OS variable (Windows_NT on Windows)
ifneq ($(filter windows_%,$(DUCKDB_PLATFORM)),)
    # Windows detected via DUCKDB_PLATFORM
    # On Windows, CMake may put executables in Release/Debug subdirectories
    # Try Release subdirectory first, fallback to direct path
    UNITTEST_RELEASE := build/release/test/Release/unittest.exe
    UNITTEST_DEBUG := build/debug/test/Debug/unittest.exe
    UNITTEST_RELDEBUG := build/reldebug/test/RelDebug/unittest.exe
else ifeq ($(OS),Windows_NT)
    # Windows detected via OS variable
    UNITTEST_RELEASE := build/release/test/Release/unittest.exe
    UNITTEST_DEBUG := build/debug/test/Debug/unittest.exe
    UNITTEST_RELDEBUG := build/reldebug/test/RelDebug/unittest.exe
else
    # Unix/Linux/MacOS (no .exe extension)
    UNITTEST_RELEASE := build/release/test/unittest
    UNITTEST_DEBUG := build/debug/test/unittest
    UNITTEST_RELDEBUG := build/reldebug/test/unittest
endif

# Override test targets to fix Windows build
# On Windows, try both possible paths (with and without Release subdirectory)
# On Unix/Linux/MacOS, use the standard path
ifneq ($(filter windows_%,$(DUCKDB_PLATFORM)),)
    # Windows: Try both possible paths
    test_release_internal:
		@if [ -f "build/release/test/unittest.exe" ]; then \
			build/release/test/unittest.exe "$(PROJ_DIR)test/*"; \
		elif [ -f "build/release/test/Release/unittest.exe" ]; then \
			build/release/test/Release/unittest.exe "$(PROJ_DIR)test/*"; \
		else \
			echo "Error: unittest.exe not found in build/release/test/ or build/release/test/Release/"; \
			exit 1; \
		fi
    test_debug_internal:
		@if [ -f "build/debug/test/unittest.exe" ]; then \
			build/debug/test/unittest.exe "$(PROJ_DIR)test/*"; \
		elif [ -f "build/debug/test/Debug/unittest.exe" ]; then \
			build/debug/test/Debug/unittest.exe "$(PROJ_DIR)test/*"; \
		else \
			echo "Error: unittest.exe not found in build/debug/test/ or build/debug/test/Debug/"; \
			exit 1; \
		fi
    test_reldebug_internal:
		@if [ -f "build/reldebug/test/unittest.exe" ]; then \
			build/reldebug/test/unittest.exe "$(PROJ_DIR)test/*"; \
		elif [ -f "build/reldebug/test/RelDebug/unittest.exe" ]; then \
			build/reldebug/test/RelDebug/unittest.exe "$(PROJ_DIR)test/*"; \
		else \
			echo "Error: unittest.exe not found in build/reldebug/test/ or build/reldebug/test/RelDebug/"; \
			exit 1; \
		fi
else ifeq ($(OS),Windows_NT)
    # Windows (fallback): Try both possible paths
    test_release_internal:
		@if [ -f "build/release/test/unittest.exe" ]; then \
			build/release/test/unittest.exe "$(PROJ_DIR)test/*"; \
		elif [ -f "build/release/test/Release/unittest.exe" ]; then \
			build/release/test/Release/unittest.exe "$(PROJ_DIR)test/*"; \
		else \
			echo "Error: unittest.exe not found in build/release/test/ or build/release/test/Release/"; \
			exit 1; \
		fi
    test_debug_internal:
		@if [ -f "build/debug/test/unittest.exe" ]; then \
			build/debug/test/unittest.exe "$(PROJ_DIR)test/*"; \
		elif [ -f "build/debug/test/Debug/unittest.exe" ]; then \
			build/debug/test/Debug/unittest.exe "$(PROJ_DIR)test/*"; \
		else \
			echo "Error: unittest.exe not found in build/debug/test/ or build/debug/test/Debug/"; \
			exit 1; \
		fi
    test_reldebug_internal:
		@if [ -f "build/reldebug/test/unittest.exe" ]; then \
			build/reldebug/test/unittest.exe "$(PROJ_DIR)test/*"; \
		elif [ -f "build/reldebug/test/RelDebug/unittest.exe" ]; then \
			build/reldebug/test/RelDebug/unittest.exe "$(PROJ_DIR)test/*"; \
		else \
			echo "Error: unittest.exe not found in build/reldebug/test/ or build/reldebug/test/RelDebug/"; \
			exit 1; \
		fi
else
    # Unix/Linux/MacOS: Use standard path (no shell logic needed)
    test_release_internal:
		$(UNITTEST_RELEASE) "$(PROJ_DIR)test/*"
    test_debug_internal:
		$(UNITTEST_DEBUG) "$(PROJ_DIR)test/*"
    test_reldebug_internal:
		$(UNITTEST_RELDEBUG) "$(PROJ_DIR)test/*"
endif

# Override the default test target to work correctly
test: release
	$(UNITTEST_RELEASE) test/sql/*

# Custom test target that works correctly
test_astro:
	$(UNITTEST_RELEASE) test/sql/* 