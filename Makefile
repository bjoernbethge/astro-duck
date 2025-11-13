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

# Set guard to allow overriding test_release_internal
HAVE_TEST_RELEASE_INTERNAL := 1

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# Override test_release_internal to depend on release build and check for binary
test_release_internal: release
	@if [ -f build/release/test/unittest ] || [ -f build/release/test/unittest.exe ]; then \
		if [ -f build/release/test/unittest ]; then \
			./build/release/test/unittest "test/*"; \
		else \
			./build/release/test/unittest.exe "$(CURDIR)/test/*"; \
		fi \
	else \
		echo "Test binary not found; run 'make release' or check build logs"; exit 1; \
	fi

# Override the default test target to work correctly
test: release
	./build/release/test/unittest test/sql/*

# Custom test target that works correctly
test_astro:
	./build/release/test/unittest test/sql/* 