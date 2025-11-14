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
ifeq ($(OS),Windows_NT)
    UNITTEST_RELEASE := build/release/test/unittest.exe
    UNITTEST_DEBUG := build/debug/test/unittest.exe
    UNITTEST_RELDEBUG := build/reldebug/test/unittest.exe
else
    UNITTEST_RELEASE := build/release/test/unittest
    UNITTEST_DEBUG := build/debug/test/unittest
    UNITTEST_RELDEBUG := build/reldebug/test/unittest
endif

# Override test targets to fix Windows build
test_release_internal:
	$(UNITTEST_RELEASE) "$(PROJ_DIR)test/*"
test_debug_internal:
	$(UNITTEST_DEBUG) "$(PROJ_DIR)test/*"
test_reldebug_internal:
	$(UNITTEST_RELDEBUG) "$(PROJ_DIR)test/*"

# Override the default test target to work correctly
test: release
	$(UNITTEST_RELEASE) test/sql/*

# Custom test target that works correctly
test_astro:
	$(UNITTEST_RELEASE) test/sql/* 