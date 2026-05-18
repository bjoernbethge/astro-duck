PROJ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# Configuration of extension
EXT_NAME=astro
EXT_CONFIG=${PROJ_DIR}extension_config.cmake

# Include the Makefile from extension-ci-tools
include extension-ci-tools/makefiles/duckdb_extension.Makefile

# --- Override test recipes for the upstream Windows test-path quoting bug ---
# extension-ci-tools/makefiles/duckdb_extension.Makefile (pin: ec20f45) defines
#   TEST_PATH="/test/unittest"          # literal quotes inside the value
#   TESTS_BASE_DIRECTORY = "test/"
# which expand to `./build/release/"/test/unittest" ""test/"*"` in the recipe.
# That happened to work on the GitHub Actions windows-latest image up to
# v1.4.1 (2026-04-18) but broke after a recent Git-for-Windows / MSYS update
# with `Error 127` (binary not found). Linux/macOS still resolve it, but the
# explicit form below is more robust on every platform, so the override is
# unconditional.
ifeq ($(OS),Windows_NT)
    UNITTEST_BIN := test/unittest.exe
else
    UNITTEST_BIN := test/unittest
endif

test_release_internal:
	./build/release/$(UNITTEST_BIN) "test/*"
test_debug_internal:
	./build/debug/$(UNITTEST_BIN) "test/*"
test_reldebug_internal:
	./build/reldebug/$(UNITTEST_BIN) "test/*"
